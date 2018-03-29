#include <vector>

#include <Poco/DateTime.h>
#include <Poco/Exception.h>
#include <Poco/NumberParser.h>
#include <Poco/PipeStream.h>
#include <Poco/StreamCopier.h>
#include <Poco/String.h>
#include <Poco/Timestamp.h>

#include "bluetooth/BluetoothAvailabilityManager.h"
#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "model/DevicePrefix.h"
#include "hotplug/HotplugEvent.h"
#include "util/PosixSignal.h"

BEEEON_OBJECT_BEGIN(BeeeOn, BluetoothAvailabilityManager)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(HotplugListener)
BEEEON_OBJECT_PROPERTY("wakeUpTime", &BluetoothAvailabilityManager::setWakeUpTime)
BEEEON_OBJECT_PROPERTY("modes", &BluetoothAvailabilityManager::setModes)
BEEEON_OBJECT_PROPERTY("distributor", &BluetoothAvailabilityManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &BluetoothAvailabilityManager::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("attemptsCount", &BluetoothAvailabilityManager::setAttemptsCount)
BEEEON_OBJECT_PROPERTY("retryTimeout", &BluetoothAvailabilityManager::setRetryTimeout)
BEEEON_OBJECT_PROPERTY("statisticsInterval", &BluetoothAvailabilityManager::setStatisticsInterval)
BEEEON_OBJECT_PROPERTY("executor", &BluetoothAvailabilityManager::setExecutor)
BEEEON_OBJECT_PROPERTY("listeners", &BluetoothAvailabilityManager::registerListener)
BEEEON_OBJECT_END(BeeeOn, BluetoothAvailabilityManager)

#define MODE_CLASSIC 0x01
#define MODE_LE 0x02

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const int MODULE_ID = 0;
const int NUM_OF_ATTEMPTS = 2;
static const Timespan SCAN_TIME = 5 * Timespan::SECONDS;
static const Timespan LE_SCAN_TIME = 5 * Timespan::SECONDS;
static const Timespan MIN_WAKE_UP_TIME = 15 * Timespan::SECONDS;

BluetoothAvailabilityManager::BluetoothAvailabilityManager() :
	DongleDeviceManager(DevicePrefix::PREFIX_BLUETOOTH),
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
	HciInterface hci(dongleName());

	fetchDeviceList();

	if (m_eventSource.asyncExecutor().isNull()) {
		logger().critical("no executor to generate statistics",
				__FILE__, __LINE__);
	}
	else {
		m_statisticsRunner.start([&]() {
			HciInterface hci(dongleName());

			const HciInfo &info = hci.info();
			m_eventSource.fireEvent(info, &BluetoothListener::onHciStats);
		});
	}

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
	while (!m_stop) {
		hci.up();

		const Timespan &remaining = detectAll(hci);

		if (remaining > 0 && !m_stop)
			m_stopEvent.tryWait(remaining.totalMilliseconds());
	}

	m_statisticsRunner.stop();
}

bool BluetoothAvailabilityManager::dongleMissing()
{
	m_statisticsRunner.stop();
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
	m_statisticsRunner.stop();
	m_leScanCache.clear();

	try {
		HciInterface hci(dongleName());
		hci.reset();
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

	m_stopEvent.set();
}

string BluetoothAvailabilityManager::dongleMatch(const HotplugEvent &e)
{
	if (e.subsystem() == "bluetooth") {
		if (e.name().empty()) {
			logger().warning("missing bluetooth interface name, skipping",
				__FILE__, __LINE__);
		}
		return e.name();
	}

	return "";
}

void BluetoothAvailabilityManager::stop()
{
	m_stopEvent.set();
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

		if (m_stop)
			break;
	}
	return inactive;
}

void BluetoothAvailabilityManager::detectLE(const HciInterface &hci)
{
	m_leScanCache.clear();
	m_leScanCache = hci.lescan(LE_SCAN_TIME);

	for (auto &device : m_deviceList) {
		if (!device.second.isLE())
			continue;

		if (m_leScanCache.find(device.second.mac()) != m_leScanCache.end())
			device.second.updateStatus(BluetoothDevice::AVAILABLE);
		else
			device.second.updateStatus(BluetoothDevice::UNAVAILABLE);

		shipStatusOf(device.second);

		if (m_stop)
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
	while (!inactive.empty() && !m_stop) {
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

bool BluetoothAvailabilityManager::accept(const Command::Ptr cmd)
{
	if (cmd->is<GatewayListenCommand>())
		return true;
	else if (cmd->is<DeviceUnpairCommand>())
		return hasDevice(cmd->cast<DeviceUnpairCommand>().deviceID());
	else if (cmd->is<DeviceAcceptCommand>())
		return cmd->cast<DeviceAcceptCommand>().deviceID().prefix() == DevicePrefix::PREFIX_BLUETOOTH;
	return false;
}

void BluetoothAvailabilityManager::handle(Command::Ptr cmd, Answer::Ptr answer)
{
	if (cmd->is<GatewayListenCommand>())
		doListenCommand(cmd, answer);
	else if (cmd->is<DeviceUnpairCommand>())
		doUnpairCommand(cmd, answer);
	else if (cmd->is<DeviceAcceptCommand>())
		doDeviceAcceptCommand(cmd, answer);
}

void BluetoothAvailabilityManager::doDeviceAcceptCommand(
	const Command::Ptr &cmd, const Answer::Ptr &answer)
{
	FastMutex::ScopedLock lock(m_lock);

	addDevice(cmd.cast<DeviceAcceptCommand>()->deviceID());

	Result::Ptr result = new Result(answer);
	result->setStatus(Result::Status::SUCCESS);
}

void BluetoothAvailabilityManager::doUnpairCommand(
	const Command::Ptr &cmd, const Answer::Ptr &answer)
{
	FastMutex::ScopedLock lock(m_lock);

	removeDevice(cmd->cast<DeviceUnpairCommand>().deviceID());

	Result::Ptr result = new Result(answer);
	result->setStatus(Result::Status::SUCCESS);
}

void BluetoothAvailabilityManager::doListenCommand(const Command::Ptr &cmd, const Answer::Ptr &answer)
{
	GatewayListenCommand::Ptr cmdListen = cmd.cast<GatewayListenCommand>();
	Result::Ptr result = new Result(answer);

	if (!m_thread.isRunning()) {
		m_listenTime = cmdListen->duration();

		try {
			m_thread.start(m_listenThread);
		}
		catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
			result->setStatus(Result::Status::FAILED);
			return;
		}
	}
	else {
		logger().debug("listen already running, dropping listen command",
			__FILE__, __LINE__);
	}

	result->setStatus(Result::Status::SUCCESS);
}

void BluetoothAvailabilityManager::fetchDeviceList()
{
	set<DeviceID> idList;
	idList = deviceList();
	m_deviceList.clear();

	FastMutex::ScopedLock lock(m_lock);

	for (const auto &id : idList)
		addDevice(id);
}

bool BluetoothAvailabilityManager::enoughTimeForScan(const Timestamp &startTime)
{
	Timespan base = 0;

	if (m_mode & MODE_CLASSIC)
		base += SCAN_TIME;
	if (m_mode & MODE_LE)
		base += LE_SCAN_TIME;

	return (base + startTime.elapsed() < m_listenTime && !m_stop);
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

		if (!hasDevice(id))
			sendNewDevice(id, scannedDevice.second);
	}
}

void BluetoothAvailabilityManager::listen()
{
	logger().information("scaning bluetooth network", __FILE__, __LINE__);

	Timestamp startTime;
	FastMutex::ScopedLock lock(m_scanLock);

	HciInterface hci(dongleName());
	hci.up();

	if (m_mode & MODE_LE)
		reportFoundDevices(MODE_LE, m_leScanCache);

	while (enoughTimeForScan(startTime)) {
		if (m_mode & MODE_CLASSIC)
			reportFoundDevices(MODE_CLASSIC, hci.scan());
		if (m_mode & MODE_LE)
			reportFoundDevices(MODE_LE, hci.lescan(LE_SCAN_TIME));
	};

	logger().information("bluetooth listen has finished", __FILE__, __LINE__);
}

void BluetoothAvailabilityManager::addDevice(const DeviceID &id)
{
	m_deviceList.emplace(id, BluetoothDevice(id));
}

bool BluetoothAvailabilityManager::hasDevice(const DeviceID &id)
{
	return m_deviceList.find(id) != m_deviceList.end();
}

void BluetoothAvailabilityManager::removeDevice(const DeviceID &id)
{
	auto it = m_deviceList.find(id);

	if (it != m_deviceList.end())
		m_deviceList.erase(it);
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
	dispatch(new NewDeviceCommand(
				id,
				"Bluetooth Availability",
				name,
				moduleTypes()));
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

void BluetoothAvailabilityManager::setStatisticsInterval(const Timespan &interval)
{
	if (interval <= 0)
		throw InvalidArgumentException("statistics interval must be a positive number");

	m_statisticsRunner.setInterval(interval);
}

void BluetoothAvailabilityManager::setExecutor(SharedPtr<AsyncExecutor> executor)
{
	m_eventSource.setAsyncExecutor(executor);
}

void BluetoothAvailabilityManager::registerListener(BluetoothListener::Ptr listener)
{
	m_eventSource.addListener(listener);
}
