#include <Poco/ScopedLock.h>

#include "bluetooth/BLESmartDeviceManager.h"
#include "bluetooth/BeeWiSmartClim.h"
#include "bluetooth/BeeWiSmartDoor.h"
#include "bluetooth/BeeWiSmartLite.h"
#include "bluetooth/BeeWiSmartMotion.h"
#include "bluetooth/BeeWiSmartWatt.h"
#include "bluetooth/HciUtil.h"
#include "bluetooth/TabuLumenSmartLite.h"
#include "bluetooth/RevogiDevice.h"
#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "hotplug/HotplugEvent.h"
#include "model/DevicePrefix.h"
#include "util/BlockingAsyncWork.h"

BEEEON_OBJECT_BEGIN(BeeeOn, BLESmartDeviceManager)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(HotplugListener)
BEEEON_OBJECT_CASTABLE(DeviceStatusHandler)
BEEEON_OBJECT_PROPERTY("deviceCache", &BLESmartDeviceManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("devicePoller", &BLESmartDeviceManager::setDevicePoller)
BEEEON_OBJECT_PROPERTY("distributor", &BLESmartDeviceManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &BLESmartDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("hciManager", &BLESmartDeviceManager::setHciManager)
BEEEON_OBJECT_PROPERTY("scanTimeout", &BLESmartDeviceManager::setScanTimeout)
BEEEON_OBJECT_PROPERTY("deviceTimeout", &BLESmartDeviceManager::setDeviceTimeout)
BEEEON_OBJECT_PROPERTY("refresh", &BLESmartDeviceManager::setRefresh)
BEEEON_OBJECT_PROPERTY("attemptsCount", &BLESmartDeviceManager::setAttemptsCount)
BEEEON_OBJECT_PROPERTY("retryTimeout", &BLESmartDeviceManager::setRetryTimeout)
BEEEON_OBJECT_END(BeeeOn, BLESmartDeviceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const UUID BLESmartDeviceManager::CHAR_MODEL_NUMBER = UUID("00002a24-0000-1000-8000-00805f9b34fb");

BLESmartDeviceManager::BLESmartDeviceManager():
	DongleDeviceManager(DevicePrefix::PREFIX_BLE_SMART, {
		typeid(GatewayListenCommand),
		typeid(DeviceAcceptCommand),
		typeid(DeviceUnpairCommand),
		typeid(DeviceSetValueCommand),
	}),
	m_scanTimeout(10 * Timespan::SECONDS),
	m_deviceTimeout(5 * Timespan::SECONDS),
	m_refresh(RefreshTime::fromSeconds(30))
{
	m_watchCallback = new HciInterface::WatchCallback(
		[&](const MACAddress& address, vector<unsigned char>& data) {
			processAsyncData(address, data);
		});
}

void BLESmartDeviceManager::setDevicePoller(DevicePoller::Ptr poller)
{
	m_pollingKeeper.setDevicePoller(poller);
}

void BLESmartDeviceManager::setScanTimeout(const Timespan &timeout)
{
	if (timeout.totalSeconds() <= 0)
		throw InvalidArgumentException("scan timeout time must be at least a second");

	m_scanTimeout = timeout;
}

void BLESmartDeviceManager::setDeviceTimeout(const Timespan &timeout)
{
	if (timeout.totalSeconds() <= 0)
		throw InvalidArgumentException("device timeout time must be at least a second");

	m_deviceTimeout = timeout;
}

void BLESmartDeviceManager::setRefresh(const Timespan &refresh)
{
	if (refresh.totalSeconds() <= 0)
		throw InvalidArgumentException("refresh time must at least a second");

	m_refresh = RefreshTime::fromSeconds(refresh.totalSeconds());
}

void BLESmartDeviceManager::setHciManager(HciInterfaceManager::Ptr manager)
{
	m_hciManager = manager;
}

void BLESmartDeviceManager::dongleAvailable()
{
	logger().information("starting BLE Smart device manager", __FILE__, __LINE__);

	m_hci = m_hciManager->lookup(dongleName());

	while (!m_stopControl.shouldStop()) {
		seekPairedDevices();

		for (auto pair : m_devices) {
			if (!pair.second->pollable())
				continue;

			if (deviceCache()->paired(pair.second->id()))
				m_pollingKeeper.schedule(pair.second);
			else
				m_pollingKeeper.cancel(pair.second->id());
		}

		m_stopControl.waitStoppable(m_refresh);
	}

	m_pollingKeeper.cancelAll();
	logger().information("stopping BLE Smart device manager", __FILE__, __LINE__);
}

void BLESmartDeviceManager::notifyDongleRemoved()
{
	m_stopControl.requestWakeup();
}

string BLESmartDeviceManager::dongleMatch(const HotplugEvent &e)
{
	return HciUtil::hotplugMatch(e);
}

void BLESmartDeviceManager::dongleFailed(const FailDetector &dongleStatus)
{
	eraseAllDevices();
	DongleDeviceManager::dongleFailed(dongleStatus);
}

void BLESmartDeviceManager::stop()
{
	DongleDeviceManager::stop();
	answerQueue().dispose();
}

bool BLESmartDeviceManager::dongleMissing()
{
	eraseAllDevices();
	return true;
}

void BLESmartDeviceManager::eraseAllDevices()
{
	ScopedLock<FastMutex> lock(m_devicesMutex);

	m_devices.clear();
}

AsyncWork<>::Ptr BLESmartDeviceManager::startDiscovery(const Timespan &timeout)
{
	BLESmartSeeker::Ptr seeker = new BLESmartSeeker(*this, timeout);
	seeker->start();
	return seeker;
}

AsyncWork<set<DeviceID>>::Ptr BLESmartDeviceManager::startUnpair(
		const DeviceID &id,
		const Timespan &timeout)
{
	auto work = BlockingAsyncWork<set<DeviceID>>::instance();

	ScopedLock<FastMutex> lock(m_devicesMutex, timeout.totalMilliseconds());

	if (!deviceCache()->paired(id)) {
		logger().warning("unpairing device that is not paired: " + id.toString(),
			__FILE__, __LINE__);
	}
	else {
		deviceCache()->markUnpaired(id);

		auto it = m_devices.find(id);
		if (it != m_devices.end()) {
			m_pollingKeeper.cancel(id);
			m_devices.erase(id);
		}

		work->setResult({id});
	}

	return work;
}

void BLESmartDeviceManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	ScopedLock<FastMutex> lock(m_devicesMutex);

	auto it = m_devices.find(cmd->deviceID());
	if (it == m_devices.end())
		throw NotFoundException("accept: " + cmd->deviceID().toString());

	it->second->pair(m_watchCallback);
	if (it->second->pollable())
		m_pollingKeeper.schedule(it->second);

	DeviceManager::handleAccept(cmd);
}

AsyncWork<double>::Ptr BLESmartDeviceManager::startSetValue(
		const DeviceID &id,
		const ModuleID &module,
		const double value,
		const Timespan &timeout)
{
	ScopedLock<FastMutex> lock(m_devicesMutex, timeout.totalMilliseconds());

	auto it = m_devices.find(id);
	if (it == m_devices.end())
		throw NotFoundException("set-value: " + id.toString());

	m_hci->up();
	it->second->requestModifyState(module, value);

	if (logger().debug()) {
		logger().debug("success to change state of device " + id.toString(),
			__FILE__, __LINE__);
	}

	auto work = BlockingAsyncWork<double>::instance();
	work->setResult(value);
	return work;
}

void BLESmartDeviceManager::processAsyncData(
		const MACAddress& address,
		std::vector<unsigned char> &data)
{
	logger().information("recieved async message from device " + address.toString(':'),
		__FILE__, __LINE__);

	ScopedLock<FastMutex> lock(m_devicesMutex);

	DeviceID id(DevicePrefix::PREFIX_BLE_SMART, address);
	auto it = m_devices.find(id);
	if (it != m_devices.end()) {
		try {
			ship(it->second->parseAdvertisingData(data));
		}
		catch (const Exception& e) {
			logger().log(e, __FILE__, __LINE__);
		}
	}
	else {
		logger().warning("recieved async message from unknown device " + address.toString(':'),
			__FILE__, __LINE__);
	}
}

void BLESmartDeviceManager::seekPairedDevices()
{
	set<DeviceID> pairedDevices;
	ScopedLockWithUnlock<FastMutex> lock(m_devicesMutex);
	for (auto &id : deviceCache()->paired(prefix())) {
		auto it = m_devices.find(id);
		if (it == m_devices.end())
			pairedDevices.insert(id);
		else
			it->second->pair(m_watchCallback);
	}
	lock.unlock();

	if (pairedDevices.size() <= 0)
		return;

	logger().information("discovering of paired BLE devices...", __FILE__, __LINE__);

	m_hci->up();
	map<MACAddress, string> foundDevices = m_hci->lescan(m_scanTimeout);

	for (const auto &device : foundDevices) {
		if (m_stopControl.shouldStop())
			break;

		DeviceID id(DevicePrefix::PREFIX_BLE_SMART, device.first);
		if (pairedDevices.find(id) == pairedDevices.end())
			continue;

		BLESmartDevice::Ptr newDevice;
		try {
			newDevice = createDevice(device.first);
			newDevice->pair(m_watchCallback);
		}
		catch (const Exception& e) {
			logger().log(e, __FILE__, __LINE__);
			continue;
		}

		ScopedLock<FastMutex> lock(m_devicesMutex);
		m_devices.emplace(newDevice->id(), newDevice);

		logger().information("found " + newDevice->productName() + " " + newDevice->id().toString(),
			__FILE__, __LINE__);
	}
}

void BLESmartDeviceManager::seekDevices(vector<BLESmartDevice::Ptr>& foundDevices, const StopControl& stop)
{
	map<MACAddress, string> devices;

	logger().information("discovering BLE devices...", __FILE__, __LINE__);

	m_hci->up();
	devices = m_hci->lescan(m_scanTimeout);

	logger().information("found " + to_string(devices.size()) + " BLE device(s)",
		__FILE__, __LINE__);

	// Only a new devices are examined.
	ScopedLockWithUnlock<FastMutex> lock(m_devicesMutex);
	map<MACAddress, string> newDevices;
	for (const auto &device : devices) {
		auto it = m_devices.find(DeviceID(DevicePrefix::PREFIX_BLE_SMART, device.first));
		if (it != m_devices.end())
			foundDevices.emplace_back(it->second);
		else
			newDevices.emplace(device);
	}
	lock.unlock();

	examineBatchOfDevices(
		newDevices,
		foundDevices,
		stop);
}

void BLESmartDeviceManager::examineBatchOfDevices(
		const map<MACAddress, string>& devices,
		vector<BLESmartDevice::Ptr>& foundDevices,
		const StopControl& stop)
{
	for (const auto &device : devices) {
		if (stop.shouldStop())
			break;

		BLESmartDevice::Ptr newDevice;
		try {
			newDevice = createDevice(device.first);
		}
		catch (const NotFoundException&) {
			// unsupported device
			continue;
		}
		catch (const Exception& e) {
			logger().log(e, __FILE__, __LINE__);
			continue;
		}
		foundDevices.push_back(newDevice);

		logger().information("found " + newDevice->productName() + " " + newDevice->id().toString(),
			__FILE__, __LINE__);
	}
}

BLESmartDevice::Ptr BLESmartDeviceManager::createDevice(const MACAddress& address) const
{
	BLESmartDevice::Ptr newDevice;
	HciConnection::Ptr conn;
	vector<unsigned char> modelIDRaw;

	try {
		conn = m_hci->connect(address, m_deviceTimeout);
		modelIDRaw = conn->read(CHAR_MODEL_NUMBER);
	}
	catch (const Exception& e) {
		throw IOException(
			"failed to read model number of device " + address.toString(), e);
	}

	string modelID = {modelIDRaw.begin(), modelIDRaw.end()};
	if (BeeWiSmartClim::match(modelID))
		newDevice = new BeeWiSmartClim(address, m_deviceTimeout, m_refresh, m_hci);
	else if (BeeWiSmartMotion::match(modelID))
		newDevice = new BeeWiSmartMotion(address, m_deviceTimeout, m_refresh, m_hci, conn);
	else if (BeeWiSmartDoor::match(modelID))
		newDevice = new BeeWiSmartDoor(address, m_deviceTimeout, m_refresh, m_hci, conn);
	else if (BeeWiSmartWatt::match(modelID))
		newDevice = new BeeWiSmartWatt(address, m_deviceTimeout, m_refresh, m_hci, conn);
	else if (BeeWiSmartLite::match(modelID))
		newDevice = new BeeWiSmartLite(address, m_deviceTimeout, m_refresh, m_hci);
	else if (TabuLumenSmartLite::match(modelID))
		newDevice = new TabuLumenSmartLite(address, m_deviceTimeout, m_refresh, m_hci);
	else if (RevogiDevice::match(modelID))
		newDevice = RevogiDevice::createDevice(address, m_deviceTimeout, m_refresh, m_hci, conn);
	else
		throw NotFoundException("device " + modelID + "not supported");

	return newDevice;
}

void BLESmartDeviceManager::processNewDevice(BLESmartDevice::Ptr newDevice)
{
	ScopedLock<FastMutex> lock(m_devicesMutex);

	m_devices.emplace(newDevice->id(), newDevice);
	if (deviceCache()->paired(newDevice->id()))
		return;

	logger().debug("found device " + newDevice->id().toString(),
		__FILE__, __LINE__);

	auto types = newDevice->moduleTypes();

	const auto description = DeviceDescription::Builder()
		.id(newDevice->id())
		.type(newDevice->vendor(), newDevice->productName())
		.modules(types)
		.refreshTime(m_refresh)
		.macAddress(newDevice->macAddress())
		.build();

	dispatch(new NewDeviceCommand(description));
}

BLESmartDeviceManager::BLESmartSeeker::BLESmartSeeker(
		BLESmartDeviceManager& parent,
		const Timespan& duration):
	AbstractSeeker(duration),
	m_parent(parent)
{
}

void BLESmartDeviceManager::BLESmartSeeker::seekLoop(StopControl &control)
{
	StopControl::Run run(control);

	while (remaining() > 0) {
		vector<BLESmartDevice::Ptr> newDevices;

		m_parent.seekDevices(newDevices, control);
		for (auto device : newDevices) {
			if (!run)
				break;

			m_parent.processNewDevice(device);
		}

		if (!run)
			break;
	}
}
