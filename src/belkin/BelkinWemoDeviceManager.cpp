#include <Poco/Net/SocketAddress.h>
#include <Poco/ScopedLock.h>
#include <Poco/Timestamp.h>

#include "belkin/BelkinWemoDeviceManager.h"
#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "commands/NewDeviceCommand.h"
#include "core/Answer.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "model/DevicePrefix.h"
#include "net/UPnP.h"

#define BELKIN_WEMO_VENDOR "Belkin WeMo"

BEEEON_OBJECT_BEGIN(BeeeOn, BelkinWemoDeviceManager)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(BelkinWemoDeviceManager)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_REF("distributor", &BelkinWemoDeviceManager::setDistributor)
BEEEON_OBJECT_REF("commandDispatcher", &BelkinWemoDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_NUMBER("upnpTimeout", &BelkinWemoDeviceManager::setUPnPTimeout)
BEEEON_OBJECT_NUMBER("httpTimeout", &BelkinWemoDeviceManager::setHTTPTimeout)
BEEEON_OBJECT_NUMBER("refresh", &BelkinWemoDeviceManager::setRefresh)
BEEEON_OBJECT_END(BeeeOn, BelkinWemoDeviceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Net;
using namespace std;

BelkinWemoDeviceManager::BelkinWemoDeviceManager():
	DeviceManager(DevicePrefix::PREFIX_BELKIN_WEMO),
	m_refresh(5 * Timespan::SECONDS),
	m_seeker(*this),
	m_httpTimeout(3 * Timespan::SECONDS),
	m_upnpTimeout(5 * Timespan::SECONDS)
{
}

void BelkinWemoDeviceManager::run()
{
	logger().information("starting Belkin WeMo device manager", __FILE__, __LINE__);

	m_pairedDevices = deviceList(-1);

	while (!m_stop) {
		Timestamp now;

		refreshPairedDevices();

		Timespan sleepTime = m_refresh - now.elapsed();
		if (sleepTime > 0)
			m_event.tryWait(sleepTime.totalMilliseconds());
	}

	logger().information("stopping Belkin WeMo device manager", __FILE__, __LINE__);

	m_seekerThread.join(m_upnpTimeout.totalMilliseconds());
	m_stop = false;
}

void BelkinWemoDeviceManager::stop()
{
	m_stop = true;
	m_event.set();
	m_seeker.stop();
}

void BelkinWemoDeviceManager::setRefresh(int secs)
{
	if (secs <= 0)
		throw InvalidArgumentException("refresh time must be a positive number");

	m_refresh = Timespan(secs * Timespan::SECONDS);
}

void BelkinWemoDeviceManager::setUPnPTimeout(int secs)
{
	if (secs <= 0)
		throw InvalidArgumentException("UPnP timeout time must be a positive number");

	m_upnpTimeout = Timespan(secs * Timespan::SECONDS);
}

void BelkinWemoDeviceManager::setHTTPTimeout(int secs)
{
	if (secs <= 0)
		throw InvalidArgumentException("http timeout time must be a positive number");

	m_httpTimeout = Timespan(secs * Timespan::SECONDS);
}

void BelkinWemoDeviceManager::refreshPairedDevices()
{
	vector<BelkinWemoSwitch> devices;

	ScopedLockWithUnlock<FastMutex> lock(m_pairedMutex);
	for (auto &id : m_pairedDevices) {
		auto it = m_switches.find(id);
		if (it == m_switches.end()) {
			logger().warning("no such device: " + id.toString(), __FILE__, __LINE__);
			continue;
		}

		devices.push_back(it->second);
	}
	lock.unlock();

	for (auto &device : devices) {
		try {
			ship(device.requestState());
		}
		catch (Exception& e) {
			logger().log(e, __FILE__, __LINE__);
			logger().warning("device " + device.deviceID().toString() + " did not answer",
				__FILE__, __LINE__);
		}
	}
}

bool BelkinWemoDeviceManager::accept(const Command::Ptr cmd)
{
	if (cmd->is<GatewayListenCommand>()) {
		return true;
	}
	else if (cmd->is<DeviceSetValueCommand>()) {
		auto it = m_pairedDevices.find(cmd->cast<DeviceSetValueCommand>().deviceID());
		return it != m_pairedDevices.end();
	}
	else if (cmd->is<DeviceUnpairCommand>()) {
		return cmd->cast<DeviceUnpairCommand>().deviceID().prefix() == DevicePrefix::PREFIX_BELKIN_WEMO;
	}
	else if (cmd->is<DeviceAcceptCommand>()) {
		return cmd->cast<DeviceAcceptCommand>().deviceID().prefix() == DevicePrefix::parse("BelkinWemo");
	}

	return false;
}

void BelkinWemoDeviceManager::handle(Command::Ptr cmd, Answer::Ptr answer)
{
	if (cmd->is<GatewayListenCommand>()) {
		doListenCommand(cmd, answer);
	}
	else if (cmd->is<DeviceSetValueCommand>()) {
		DeviceSetValueCommand::Ptr cmdSet = cmd.cast<DeviceSetValueCommand>();

		Result::Ptr result = new Result(answer);

		if (modifyValue(cmdSet->deviceID(), cmdSet->moduleID(), cmdSet->value())) {
			result->setStatus(Result::Status::SUCCESS);
			logger().debug("success to change state of switch " + cmdSet->deviceID().toString(),
				__FILE__, __LINE__);
		}
		else {
			result->setStatus(Result::Status::FAILED);
			logger().debug("failed to change state of switch " + cmdSet->deviceID().toString(),
				__FILE__, __LINE__);
		}
	}
	else if (cmd->is<DeviceUnpairCommand>()) {
		doUnpairCommand(cmd, answer);
	}
	else if (cmd->is<DeviceAcceptCommand>()) {
		doDeviceAcceptCommand(cmd, answer);
	}
}

void BelkinWemoDeviceManager::doListenCommand(const Command::Ptr cmd, const Answer::Ptr answer)
{
	GatewayListenCommand::Ptr cmdListen = cmd.cast<GatewayListenCommand>();

	Result::Ptr result = new Result(answer);

	if (!m_seekerThread.isRunning()) {
		try {
			m_seeker.setDuration(cmdListen->duration());
			m_seekerThread.start(m_seeker);
		}
		catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
			logger().error("listening thread failed to start", __FILE__, __LINE__);
			result->setStatus(Result::Status::FAILED);
			return;
		}
	}
	else {
		logger().warning("listen seems to be running already, dropping listen command", __FILE__, __LINE__);
	}

	result->setStatus(Result::Status::SUCCESS);
}

void BelkinWemoDeviceManager::doUnpairCommand(const Command::Ptr cmd, const Answer::Ptr answer)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	DeviceUnpairCommand::Ptr cmdUnpair = cmd.cast<DeviceUnpairCommand>();

	auto it = m_pairedDevices.find(cmdUnpair->deviceID());
	if (it == m_pairedDevices.end()) {
		logger().warning("unpairing device that is not paired: " + cmdUnpair->deviceID().toString(),
			__FILE__, __LINE__);
	}
	else {
		m_pairedDevices.erase(it);
		m_switches.erase(cmdUnpair->deviceID());
	}

	Result::Ptr result = new Result(answer);
	result->setStatus(Result::Status::SUCCESS);
}

void BelkinWemoDeviceManager::doDeviceAcceptCommand(const Command::Ptr cmd, const Answer::Ptr answer)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	DeviceAcceptCommand::Ptr cmdAccept = cmd.cast<DeviceAcceptCommand>();
	Result::Ptr result = new Result(answer);

	auto it = m_switches.find(cmdAccept->deviceID());
	if (it == m_switches.end()) {
		logger().warning("not accepting device that is not found: " + cmdAccept->deviceID().toString(),
			__FILE__, __LINE__);
		result->setStatus(Result::Status::FAILED);
		return;
	}

	m_pairedDevices.insert(cmdAccept->deviceID());

	result->setStatus(Result::Status::SUCCESS);
}

bool BelkinWemoDeviceManager::modifyValue(const DeviceID& deviceID,
	const ModuleID& moduleID, const double value)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	try {
		auto it = m_switches.find(deviceID);
		if (it == m_switches.end()) {
			logger().warning("no such device: " + deviceID.toString(), __FILE__, __LINE__);
			return false;
		}
		else {
			return it->second.requestModifyState(moduleID, value);
		}
	}
	catch (const Exception& e) {
		logger().log(e, __FILE__, __LINE__);
		logger().warning("failed to change state of switch " + deviceID.toString(),
			__FILE__, __LINE__);
	}
	catch (...) {
		logger().critical("unexpected exception", __FILE__, __LINE__);
	}

	return false;
}

vector<BelkinWemoSwitch> BelkinWemoDeviceManager::seekSwitches()
{
	UPnP upnp;
	list<SocketAddress> listOfDevices;
	vector<BelkinWemoSwitch> devices;

	listOfDevices = upnp.discover(m_upnpTimeout, "urn:Belkin:device:controllee:1");
	for (const auto &address : listOfDevices) {
		if (m_stop)
			break;

		BelkinWemoSwitch newDevice;
		try {
			newDevice = BelkinWemoSwitch::buildDevice(address, m_httpTimeout);
		}
		catch (const TimeoutException& e) {
			logger().debug("found device has disconnected", __FILE__, __LINE__);
			continue;
		}

		devices.push_back(newDevice);
	}

	return devices;
}

void BelkinWemoDeviceManager::processNewDevice(BelkinWemoSwitch& newDevice)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	/*
	 * Finds out if the device is already added.
	 * If the device already exists but has different IP address
	 * update the device.
	 */
	auto it = m_switches.emplace(newDevice.deviceID(), newDevice);
	if (!it.second) {
		it.first->second.setAddress(newDevice.address());
		return;
	}

	logger().debug("found device " + newDevice.deviceID().toString() +
		" at " + newDevice.address().toString(), __FILE__, __LINE__);

	auto types = newDevice.moduleTypes();
	dispatch(
		new NewDeviceCommand(
			newDevice.deviceID(),
			BELKIN_WEMO_VENDOR,
			newDevice.name(),
			types));
}

BelkinWemoDeviceManager::BelkinWemoSeeker::BelkinWemoSeeker(BelkinWemoDeviceManager& parent) :
	m_parent(parent),
	m_stop(false)
{
}

void BelkinWemoDeviceManager::BelkinWemoSeeker::setDuration(const Poco::Timespan& duration)
{
	m_duration = duration;
}

void BelkinWemoDeviceManager::BelkinWemoSeeker::run()
{
	Timestamp now;

	while (now.elapsed() < m_duration.totalMicroseconds()) {
		for (auto device : m_parent.seekSwitches()) {
			if (m_stop)
				break;

			m_parent.processNewDevice(device);
		}

		if (m_stop)
			break;
	}

	m_stop = false;
}

void BelkinWemoDeviceManager::BelkinWemoSeeker::stop()
{
	m_stop = true;
}
