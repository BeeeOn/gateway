#include <Poco/ScopedLock.h>

#include "bluetooth/BLESmartDeviceManager.h"
#include "bluetooth/BeeWiSmartClim.h"
#include "bluetooth/BeeWiSmartDoor.h"
#include "bluetooth/BeeWiSmartMotion.h"
#include "bluetooth/BeeWiSmartWatt.h"
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
BEEEON_OBJECT_PROPERTY("deviceCache", &BLESmartDeviceManager::setDeviceCache)
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
	m_refresh(30 * Timespan::SECONDS)
{
	m_watchCallback = new HciInterface::WatchCallback(
		[&](const MACAddress& address, vector<unsigned char>& data) {
			processAsyncData(address, data);
		});

	m_matchedNames = {
		"unknown",
		BeeWiSmartClim::NAME,
		BeeWiSmartMotion::NAME,
		BeeWiSmartDoor::NAME,
		BeeWiSmartWatt::NAME,
	};
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

	m_refresh = refresh;
}

void BLESmartDeviceManager::setHciManager(HciInterfaceManager::Ptr manager)
{
	m_hciManager = manager;
}

void BLESmartDeviceManager::dongleAvailable()
{
	logger().information("starting BLE Smart device manager", __FILE__, __LINE__);

	m_hci = m_hciManager->lookup(dongleName());

	set<DeviceID> paired;
	try {
		paired = deviceList(-1);
	}
	catch (Exception& e) {
		logger().log(e, __FILE__, __LINE__);
	}

	handleRemoteStatus(prefix(), paired);

	while (!m_stopControl.shouldStop()) {
		Timestamp now;

		refreshPairedDevices();

		Timespan sleepTime = m_refresh - now.elapsed();
		m_stopControl.waitStoppable(sleepTime);
	}

	logger().information("stopping BLE Smart device manager", __FILE__, __LINE__);
}

void BLESmartDeviceManager::notifyDongleRemoved()
{
	m_stopControl.requestWakeup();
}

string BLESmartDeviceManager::dongleMatch(const HotplugEvent &e)
{
	if (e.subsystem() == "bluetooth" && e.type() == "host") {
		if (e.name().empty()) {
			logger().warning("missing bluetooth interface name, skipping",
				__FILE__, __LINE__);
		}
		return e.name();
	}

	return "";
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

void BLESmartDeviceManager::refreshPairedDevices()
{
	vector<BLESmartDevice::Ptr> devices;
	set<DeviceID> notFoundDevices;
	m_hci->up();

	ScopedLockWithUnlock<FastMutex> lock(m_devicesMutex);
	for (auto &id : deviceCache()->paired(prefix())) {
		auto it = m_devices.find(id);
		if (it == m_devices.end())
			notFoundDevices.insert(id);
		else
			devices.push_back(it->second);
	}
	lock.unlock();

	if (notFoundDevices.size() > 0)
		seekPairedDevices(notFoundDevices, devices);

	for (auto &device : devices) {
		try {
			ship(device->requestState(m_hci));
		}
		catch (const NotImplementedException& e) {
			// Some devices do not support getting the actual values.
			continue;
		}
		catch (const Exception& e) {
			logger().log(e, __FILE__, __LINE__);
			logger().warning("failed to obtain data from device " + device->deviceID().toString(),
				__FILE__, __LINE__);
		}
	}
}

void BLESmartDeviceManager::eraseAllDevices()
{
	ScopedLock<FastMutex> lock(m_devicesMutex);

	m_devices.clear();
}

void BLESmartDeviceManager::handleRemoteStatus(
		const DevicePrefix&,
		const std::set<DeviceID> &paired)
{
	for (const auto &id : paired)
		deviceCache()->markPaired(id);
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
		if (it != m_devices.end())
			m_devices.erase(id);

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

	it->second->pair(m_hci, m_watchCallback);

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
	it->second->requestModifyState(module, value, m_hci);

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

void BLESmartDeviceManager::seekPairedDevices(
		const set<DeviceID> pairedDevices,
		vector<BLESmartDevice::Ptr>& devices)
{
	logger().information("discovering of paired BLE devices...", __FILE__, __LINE__);

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
			newDevice->pair(m_hci, m_watchCallback);
		}
		catch (const Exception& e) {
			logger().log(e, __FILE__, __LINE__);
			continue;
		}

		ScopedLock<FastMutex> lock(m_devicesMutex);
		m_devices.emplace(newDevice->deviceID(), newDevice);
		devices.push_back(newDevice);

		logger().information("found " + newDevice->productName() + " " + newDevice->deviceID().toString(),
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

	for (const auto &device : devices) {
		if (stop.shouldStop())
			break;

		if (!fastPrefilter(device.second))
			continue;

		ScopedLockWithUnlock<FastMutex> lock(m_devicesMutex);
		auto it = m_devices.find(DeviceID(DevicePrefix::PREFIX_BLE_SMART, device.first));
		if (it != m_devices.end()) {
			foundDevices.push_back(it->second);
			logger().information("found " + device.second + " " + it->first.toString(),
				__FILE__, __LINE__);
			continue;
		}
		lock.unlock();

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

		logger().information("found " + newDevice->productName() + " " + newDevice->deviceID().toString(),
			__FILE__, __LINE__);
	}
}

bool BLESmartDeviceManager::fastPrefilter(const string& name) const
{
	return m_matchedNames.find(name) != m_matchedNames.end();
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
		newDevice = new BeeWiSmartClim(address, m_deviceTimeout);
	else if (BeeWiSmartMotion::match(modelID))
		newDevice = new BeeWiSmartMotion(address, m_deviceTimeout, conn);
	else if (BeeWiSmartDoor::match(modelID))
		newDevice = new BeeWiSmartDoor(address, m_deviceTimeout, conn);
	else if (BeeWiSmartWatt::match(modelID))
		newDevice = new BeeWiSmartWatt(address, m_deviceTimeout, conn);
	else
		throw NotFoundException("device " + modelID + "not supported");

	return newDevice;
}

void BLESmartDeviceManager::processNewDevice(BLESmartDevice::Ptr newDevice)
{
	ScopedLock<FastMutex> lock(m_devicesMutex);

	auto it = m_devices.emplace(newDevice->deviceID(), newDevice);
	if (!it.second)
		return;

	logger().debug("found device " + newDevice->deviceID().toString(),
		__FILE__, __LINE__);

	auto types = newDevice->moduleTypes();
	dispatch(
		new NewDeviceCommand(
			newDevice->deviceID(),
			newDevice->vendor(),
			newDevice->productName(),
			types,
			m_refresh));
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
