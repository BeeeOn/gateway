#include <vector>

#include <Poco/DateTime.h>
#include <Poco/Exception.h>
#include <Poco/NumberParser.h>
#include <Poco/PipeStream.h>
#include <Poco/StreamCopier.h>
#include <Poco/String.h>
#include <Poco/Timestamp.h>

#include "bluetooth/BluetoothAvailabilityManager.h"
#include "bluetooth/HciUtil.h"
#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "model/DevicePrefix.h"
#include "hotplug/HotplugEvent.h"
#include "util/PosixSignal.h"
#include "util/BlockingAsyncWork.h"
#include "util/ThreadWrapperAsyncWork.h"

BEEEON_OBJECT_BEGIN(BeeeOn, BluetoothAvailabilityManager)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(HotplugListener)
BEEEON_OBJECT_CASTABLE(DeviceStatusHandler)
BEEEON_OBJECT_PROPERTY("deviceCache", &BluetoothAvailabilityManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("wakeUpTime", &BluetoothAvailabilityManager::setWakeUpTime)
BEEEON_OBJECT_PROPERTY("leScanTime", &BluetoothAvailabilityManager::setLEScanTime)
BEEEON_OBJECT_PROPERTY("modes", &BluetoothAvailabilityManager::setModes)
BEEEON_OBJECT_PROPERTY("distributor", &BluetoothAvailabilityManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &BluetoothAvailabilityManager::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("hciManager", &BluetoothAvailabilityManager::setHciManager)
BEEEON_OBJECT_PROPERTY("attemptsCount", &BluetoothAvailabilityManager::setAttemptsCount)
BEEEON_OBJECT_PROPERTY("retryTimeout", &BluetoothAvailabilityManager::setRetryTimeout)
BEEEON_OBJECT_END(BeeeOn, BluetoothAvailabilityManager)

#define MODE_CLASSIC 0x01
#define MODE_LE 0x02

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const int MODULE_ID = 0;
const int NUM_OF_ATTEMPTS = 2;
static const Timespan SCAN_TIME = 5 * Timespan::SECONDS;
static const Timespan MIN_WAKE_UP_TIME = 15 * Timespan::SECONDS;

BluetoothAvailabilityManager::BluetoothAvailabilityManager() :
	DongleDeviceManager(DevicePrefix::PREFIX_BLUETOOTH, {
		typeid(GatewayListenCommand),
		typeid(DeviceUnpairCommand),
		typeid(DeviceAcceptCommand),
	}),
	m_listenThread(*this, &BluetoothAvailabilityManager::listen),
	m_wakeUpTime(MIN_WAKE_UP_TIME),
	m_listenTime(0),
	m_mode(MODE_CLASSIC)
{
}

void BluetoothAvailabilityManager::setWakeUpTime(const Timespan &time)
{
	if(time < MIN_WAKE_UP_TIME) {
		throw InvalidArgumentException("wakeUpTime must not be smaller then "
			+ to_string(MIN_WAKE_UP_TIME.totalSeconds()) + " s");
	}

	m_wakeUpTime = time;
}

void BluetoothAvailabilityManager::setLEScanTime(const Timespan &time)
{
	if (time.totalSeconds() <= 0)
		throw InvalidArgumentException("LE scan time must be at least a second");

	m_leScanTime = time;
}

void BluetoothAvailabilityManager::setModes(const list<string> &modes)
{
	m_mode = 0;

	if (find(modes.begin(), modes.end(), "classic") != modes.end())
		m_mode |= MODE_CLASSIC;
	if (find(modes.begin(), modes.end(), "le") != modes.end())
		m_mode |= MODE_LE;
}

void BluetoothAvailabilityManager::dongleAvailable()
{
	HciInterface::Ptr hci = m_hciManager->lookup(dongleName());

	fetchDeviceList();

	/*
	 * Scanning of a single device.
	 * ~5 seconds when it's unavailable,
	 * 2~3 seconds when it's available.
	 *
	 * To make the scanning more effective, we scan active devices
	 * once and the rest of the "sleeping" time, we scan only those
	 * devices that seem to be unavailable. This way, we scan
	 * unavailable devices more often but only while we fit into the
	 * "sleeping" period (wake up time).
	 */
	while (!m_stopControl.shouldStop()) {
		hci->up();

		const Timespan &remaining = detectAll(*hci);

		if (remaining > 0 && !m_stopControl.shouldStop())
			m_stopControl.waitStoppable(remaining);
	}
}

bool BluetoothAvailabilityManager::dongleMissing()
{
	m_leScanCache.clear();

	if (!m_deviceList.empty()) {
		for (auto &device : m_deviceList) {
			device.second.updateStatus(BluetoothDevice::UNKNOWN);
			shipStatusOf(device.second);
		}

		m_deviceList.clear();
	}
	return true;
}

void BluetoothAvailabilityManager::dongleFailed(const FailDetector &dongleStatus)
{
	m_leScanCache.clear();

	try {
		HciInterface::Ptr hci = m_hciManager->lookup(dongleName());
		hci->reset();
	}
	catch (const Exception &e) {
		logger().log(e, __FILE__, __LINE__);
	}

	DongleDeviceManager::dongleFailed(dongleStatus);
}

void BluetoothAvailabilityManager::notifyDongleRemoved()
{
	if (m_thread.isRunning()) {
		logger().warning("forcing listen thread to finish",
				__FILE__, __LINE__);
		PosixSignal::send(m_thread, "SIGUSR1");
	}

	m_stopControl.requestWakeup();
}

string BluetoothAvailabilityManager::dongleMatch(const HotplugEvent &e)
{
	return HciUtil::hotplugMatch(e);
}

void BluetoothAvailabilityManager::stop()
{
	DongleDeviceManager::stop();
	answerQueue().dispose();
}

list<DeviceID> BluetoothAvailabilityManager::detectClassic(const HciInterface &hci)
{
	list<DeviceID> inactive;

	for (auto &device : m_deviceList) {
		if (!device.second.isClassic())
			continue;

		if (hci.detect(device.second.mac())) {
			device.second.updateStatus(BluetoothDevice::AVAILABLE);
			shipStatusOf(device.second);
		}
		else {
			if (device.second.status() == BluetoothDevice::UNAVAILABLE)
				shipStatusOf(device.second);
			else
				inactive.push_back(device.first);
		}

		if (m_stopControl.shouldStop())
			break;
	}
	return inactive;
}

void BluetoothAvailabilityManager::detectLE(const HciInterface &hci)
{
	m_leScanCache.clear();
	m_leScanCache = hci.lescan(m_leScanTime);

	for (auto &device : m_deviceList) {
		if (!device.second.isLE())
			continue;

		if (m_leScanCache.find(device.second.mac()) != m_leScanCache.end())
			device.second.updateStatus(BluetoothDevice::AVAILABLE);
		else
			device.second.updateStatus(BluetoothDevice::UNAVAILABLE);

		shipStatusOf(device.second);

		if (m_stopControl.shouldStop())
			break;
	}
}

Timespan BluetoothAvailabilityManager::detectAll(const HciInterface &hci)
{
	FastMutex::ScopedLock lock(m_scanLock);
	Timestamp startTime;
	list<DeviceID> inactive;

	if (m_mode & MODE_CLASSIC)
		inactive = detectClassic(hci);
	if (m_mode & MODE_LE)
		detectLE(hci);

	/*
	 * Now, scan only devices that seem to be inactive
	 * (unavailable). Some of them might respond early.
	 * Scan until the "sleeping" period is done.
	 *
	 * When a device is detected again, ship the information
	 * immediately.
	 */
	while (!inactive.empty() && !m_stopControl.shouldStop()) {
		if (!haveTimeForInactive(startTime.elapsed()))
			break;

		auto it = inactive.begin();

		while (it != inactive.end()) {
			auto &dev = m_deviceList.at(*it);

			if (!hci.detect(dev.mac())) {
				++it;
				continue;
			}

			dev.updateStatus(BluetoothDevice::AVAILABLE);
			shipStatusOf(dev);
			it = inactive.erase(it);
		}
	}

	/*
	 * Devices that are still inactive are shipped as
	 * unavailable.
	 */
	for (auto id : inactive) {
		m_deviceList.at(id).updateStatus(BluetoothDevice::UNAVAILABLE);
		shipStatusOf(m_deviceList.at(id));
	}

	return m_wakeUpTime - startTime.elapsed();
}

bool BluetoothAvailabilityManager::haveTimeForInactive(Timespan elapsedTime)
{
	Timespan tryTime = m_wakeUpTime - elapsedTime - SCAN_TIME;

	return tryTime > 0;
}

void BluetoothAvailabilityManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	FastMutex::ScopedLock lock(m_lock);

	const auto id = cmd->deviceID();
	m_deviceList.emplace(id, BluetoothDevice(id));
	deviceCache()->markPaired(id);
}

AsyncWork<>::Ptr BluetoothAvailabilityManager::startDiscovery(const Timespan &timeout)
{
	m_listenTime = timeout;
	m_thread.start(m_listenThread);

	return new ThreadWrapperAsyncWork<>(m_thread);
}

AsyncWork<set<DeviceID>>::Ptr BluetoothAvailabilityManager::startUnpair(
		const DeviceID &id,
		const Timespan &)
{
	FastMutex::ScopedLock lock(m_lock);

	removeDevice(id);

	auto work = BlockingAsyncWork<set<DeviceID>>::instance();
	work->setResult({id});

	return work;
}

void BluetoothAvailabilityManager::fetchDeviceList()
{
	set<DeviceID> idList = waitRemoteStatus(-1);

	FastMutex::ScopedLock lock(m_lock);

	m_deviceList.clear();

	for (const auto &id : idList)
		m_deviceList.emplace(id, BluetoothDevice(id));
}

bool BluetoothAvailabilityManager::enoughTimeForScan(const Timestamp &startTime)
{
	Timespan base = 0;

	if (m_mode & MODE_CLASSIC)
		base += SCAN_TIME;
	if (m_mode & MODE_LE)
		base += m_leScanTime;

	return (base + startTime.elapsed() < m_listenTime && !m_stopControl.shouldStop());
}

void BluetoothAvailabilityManager::reportFoundDevices(
	const int mode, const map<MACAddress, string> &devices)
{
	for (const auto &scannedDevice : devices) {
		DeviceID id;

		if (mode == MODE_CLASSIC)
			id = createDeviceID(scannedDevice.first);
		else if (mode == MODE_LE)
			id = createLEDeviceID(scannedDevice.first);
		else
			return;

		if (!deviceCache()->paired(id))
			sendNewDevice(id, scannedDevice.second);
	}
}

void BluetoothAvailabilityManager::listen()
{
	logger().information("scaning bluetooth network", __FILE__, __LINE__);

	Timestamp startTime;
	FastMutex::ScopedLock lock(m_scanLock);

	HciInterface::Ptr hci = m_hciManager->lookup(dongleName());
	hci->up();

	if (m_mode & MODE_LE)
		reportFoundDevices(MODE_LE, m_leScanCache);

	while (enoughTimeForScan(startTime)) {
		if (m_mode & MODE_CLASSIC)
			reportFoundDevices(MODE_CLASSIC, hci->scan());
		if (m_mode & MODE_LE)
			reportFoundDevices(MODE_LE, hci->lescan(m_leScanTime));
	};

	logger().information("bluetooth listen has finished", __FILE__, __LINE__);
}

void BluetoothAvailabilityManager::removeDevice(const DeviceID &id)
{
	auto it = m_deviceList.find(id);

	if (it != m_deviceList.end())
		m_deviceList.erase(it);

	deviceCache()->markUnpaired(id);
}

void BluetoothAvailabilityManager::shipStatusOf(const BluetoothDevice &device)
{
	SensorData data;
	data.setDeviceID(device.deviceID());

	switch (device.status()) {
	case BluetoothDevice::AVAILABLE:
		data.insertValue(SensorValue(ModuleID(MODULE_ID), 1.0));
		break;
	case BluetoothDevice::UNAVAILABLE:
		data.insertValue(SensorValue(ModuleID(MODULE_ID), 0.0));
		break;
	default:
		data.insertValue(SensorValue(ModuleID(MODULE_ID)));
	}

	ship(data);
}

void BluetoothAvailabilityManager::sendNewDevice(const DeviceID &id, const string &name)
{
	logger().debug("new device: id = " + id.toString() + " name = " + name);

	const auto description = DeviceDescription::Builder()
		.id(id)
		.type("Bluetooth Availability", name)
		.modules(moduleTypes())
		.noRefreshTime()
		.build();

	dispatch(new NewDeviceCommand(description));
}

list<ModuleType> BluetoothAvailabilityManager::moduleTypes() const
{
	list<ModuleType> moduleTypes;
	moduleTypes.push_back(
		ModuleType(ModuleType::Type::TYPE_AVAILABILITY)
	);

	return moduleTypes;
}


DeviceID BluetoothAvailabilityManager::createDeviceID(const MACAddress &numMac) const
{
	return DeviceID(DevicePrefix::PREFIX_BLUETOOTH, numMac.toNumber());
}

DeviceID BluetoothAvailabilityManager::createLEDeviceID(const MACAddress &numMac) const
{
	return DeviceID(DevicePrefix::PREFIX_BLUETOOTH, numMac.toNumber() |
		BluetoothDevice::DEVICE_ID_LE_MASK);
}

void BluetoothAvailabilityManager::setHciManager(HciInterfaceManager::Ptr manager)
{
	m_hciManager = manager;
}
