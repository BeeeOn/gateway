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
BEEEON_OBJECT_NUMBER("wakeUpTime", &BluetoothAvailabilityManager::setWakeUpTime)
BEEEON_OBJECT_REF("distributor", &BluetoothAvailabilityManager::setDistributor)
BEEEON_OBJECT_REF("commandDispatcher", &BluetoothAvailabilityManager::setCommandDispatcher)
BEEEON_OBJECT_NUMBER("attemptsCount", &BluetoothAvailabilityManager::setAttemptsCount)
BEEEON_OBJECT_NUMBER("retryTimeout", &BluetoothAvailabilityManager::setRetryTimeout)
BEEEON_OBJECT_NUMBER("failTimeout", &BluetoothAvailabilityManager::setFailTimeout)
BEEEON_OBJECT_END(BeeeOn, BluetoothAvailabilityManager)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const int MODULE_ID = 0;
const int NUM_OF_ATTEMPTS = 2;
static const Timespan SCAN_TIME = 5 * Timespan::SECONDS;
static const Timespan MIN_WAKE_UP_TIME = 15 * Timespan::SECONDS;

BluetoothAvailabilityManager::BluetoothAvailabilityManager() :
	DongleDeviceManager(DevicePrefix::PREFIX_BLUETOOTH),
	m_listenThread(*this, &BluetoothAvailabilityManager::listen),
	m_wakeUpTime(MIN_WAKE_UP_TIME)
{
}

void BluetoothAvailabilityManager::setWakeUpTime(int time)
{
	if(time < MIN_WAKE_UP_TIME.totalSeconds()) {
		throw InvalidArgumentException("wakeUpTime must not be smaller then "
			+ to_string(MIN_WAKE_UP_TIME.totalSeconds()));
	}

	m_wakeUpTime = Timespan(time * Timespan::SECONDS);
}

void BluetoothAvailabilityManager::dongleAvailable()
{
	HciInterface hci(dongleName());
	hci.up();

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
	while (!m_stop) {
		const Timespan &remaining = detectAll(hci);

		if (remaining > 0 && !m_stop)
			m_stopEvent.tryWait(remaining.totalMilliseconds());
	}
}

bool BluetoothAvailabilityManager::dongleMissing()
{
	if (!m_deviceList.empty()) {
		for (auto &device : m_deviceList) {
			device.second.updateStatus(BluetoothDevice::UNKNOWN);
			shipStatusOf(device.second);
		}

		m_deviceList.clear();
	}
	return true;
}

void BluetoothAvailabilityManager::notifyDongleRemoved()
{
	if (m_thread.isRunning()) {
		logger().warning("forcing listen thread to finish",
				__FILE__, __LINE__);
		PosixSignal::send(m_thread, "SIGUSR1");
	}
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

Timespan BluetoothAvailabilityManager::detectAll(const HciInterface &hci)
{
	Timestamp startTime;
	list<DeviceID> inactive;

	/*
	 * Scan for all paired devices. Any inactive device is
	 * temporarily saved. Information about active devices
	 * is immediatelly shipped.
	 */
	for (auto &device : m_deviceList) {
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

void BluetoothAvailabilityManager::listen()
{
	logger().information("scaning bluetooth network", __FILE__, __LINE__);

	HciInterface hci(dongleName());
	hci.up();

	for (const auto &scannedDevice : hci.scan()) {
		DeviceID id = createDeviceID(scannedDevice.second);
		if (!hasDevice(id))
			sendNewDevice(id, scannedDevice.first);
	}
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
