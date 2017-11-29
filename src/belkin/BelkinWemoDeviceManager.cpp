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
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_REF("distributor", &BelkinWemoDeviceManager::setDistributor)
BEEEON_OBJECT_REF("commandDispatcher", &BelkinWemoDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_TIME("upnpTimeout", &BelkinWemoDeviceManager::setUPnPTimeout)
BEEEON_OBJECT_TIME("httpTimeout", &BelkinWemoDeviceManager::setHTTPTimeout)
BEEEON_OBJECT_TIME("refresh", &BelkinWemoDeviceManager::setRefresh)
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
	if (m_pairedDevices.size() > 0)
		searchPairedDevices();

	while (!m_stop) {
		Timestamp now;

		eraseUnusedLinks();

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
	answerQueue().dispose();
}

void BelkinWemoDeviceManager::setRefresh(const Timespan &refresh)
{
	if (refresh.totalSeconds() <= 0)
		throw InvalidArgumentException("refresh time must at least a second");

	m_refresh = refresh;
}

void BelkinWemoDeviceManager::setUPnPTimeout(const Timespan &timeout)
{
	if (timeout.totalSeconds() <= 0)
		throw InvalidArgumentException("UPnP timeout time must be at least a second");

	m_upnpTimeout = timeout;
}

void BelkinWemoDeviceManager::setHTTPTimeout(const Timespan &timeout)
{
	if (timeout.totalSeconds() <= 0)
		throw InvalidArgumentException("http timeout time must be at least a second");

	m_httpTimeout = timeout;
}

void BelkinWemoDeviceManager::refreshPairedDevices()
{
	vector<BelkinWemoDevice::Ptr> devices;

	ScopedLockWithUnlock<FastMutex> lock(m_pairedMutex);
	for (auto &id : m_pairedDevices) {
		auto it = m_devices.find(id);
		if (it == m_devices.end()) {
			logger().warning("no such device: " + id.toString(), __FILE__, __LINE__);
			continue;
		}

		devices.push_back(it->second);
	}
	lock.unlock();

	for (auto &device : devices) {
		try {
			ScopedLock<FastMutex> guard(device->lock());
			ship(device->requestState());
		}
		catch (Exception& e) {
			logger().log(e, __FILE__, __LINE__);
			logger().warning("device " + device->deviceID().toString() + " did not answer",
				__FILE__, __LINE__);
		}
	}
}

void BelkinWemoDeviceManager::searchPairedDevices()
{
	vector<BelkinWemoSwitch::Ptr> switches = seekSwitches();

	ScopedLockWithUnlock<FastMutex> lockSwitch(m_pairedMutex);
	for (auto device : switches) {
		auto it = m_pairedDevices.find(device->deviceID());
		if (it != m_pairedDevices.end())
			m_devices.emplace(device->deviceID(), device);
	}
	lockSwitch.unlock();

	vector<BelkinWemoBulb::Ptr> bulbs = seekBulbs();

	ScopedLockWithUnlock<FastMutex> lockBulb(m_pairedMutex);
	for (auto device : bulbs) {
		auto it = m_pairedDevices.find(device->deviceID());
		if (it != m_pairedDevices.end())
			m_devices.emplace(device->deviceID(), device);
	}
	lockBulb.unlock();
}

void BelkinWemoDeviceManager::eraseUnusedLinks()
{
	ScopedLock<FastMutex> lock(m_linksMutex);
	vector<BelkinWemoLink::Ptr> links;

	for (auto link : m_links) {
		if (link.second->countOfBulbs() == 0)
			links.push_back(link.second);
	}

	for (auto link : links) {
		logger().debug("erase Belkin Wemo Link " + link->macAddress().toString(), __FILE__, __LINE__);

		m_links.erase(link->macAddress());
	}
}

bool BelkinWemoDeviceManager::accept(const Command::Ptr cmd)
{
	if (cmd->is<GatewayListenCommand>()) {
		return true;
	}
	else if (cmd->is<DeviceSetValueCommand>()) {
		return cmd->cast<DeviceSetValueCommand>().deviceID().prefix() == DevicePrefix::PREFIX_BELKIN_WEMO;
	}
	else if (cmd->is<DeviceUnpairCommand>()) {
		return cmd->cast<DeviceUnpairCommand>().deviceID().prefix() == DevicePrefix::PREFIX_BELKIN_WEMO;
	}
	else if (cmd->is<DeviceAcceptCommand>()) {
		return cmd->cast<DeviceAcceptCommand>().deviceID().prefix() == DevicePrefix::PREFIX_BELKIN_WEMO;
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
			logger().debug("success to change state of device " + cmdSet->deviceID().toString(),
				__FILE__, __LINE__);
		}
		else {
			result->setStatus(Result::Status::FAILED);
			logger().debug("failed to change state of device " + cmdSet->deviceID().toString(),
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

		auto itDevice = m_devices.find(cmdUnpair->deviceID());
		if (itDevice != m_devices.end())
			m_devices.erase(cmdUnpair->deviceID());
	}

	Result::Ptr result = new Result(answer);
	result->setStatus(Result::Status::SUCCESS);
}

void BelkinWemoDeviceManager::doDeviceAcceptCommand(const Command::Ptr cmd, const Answer::Ptr answer)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	DeviceAcceptCommand::Ptr cmdAccept = cmd.cast<DeviceAcceptCommand>();
	Result::Ptr result = new Result(answer);

	auto it = m_devices.find(cmdAccept->deviceID());
	if (it == m_devices.end()) {
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
		auto it = m_devices.find(deviceID);
		if (it == m_devices.end()) {
			logger().warning("no such device: " + deviceID.toString(), __FILE__, __LINE__);
			return false;
		}
		else {
			ScopedLock<FastMutex> guard(it->second->lock());
			return it->second->requestModifyState(moduleID, value);
		}
	}
	catch (const Exception& e) {
		logger().log(e, __FILE__, __LINE__);
		logger().warning("failed to change state of device " + deviceID.toString(),
			__FILE__, __LINE__);
	}
	catch (...) {
		logger().critical("unexpected exception", __FILE__, __LINE__);
	}

	return false;
}

vector<BelkinWemoSwitch::Ptr> BelkinWemoDeviceManager::seekSwitches()
{
	UPnP upnp;
	list<SocketAddress> listOfDevices;
	vector<BelkinWemoSwitch::Ptr> devices;

	listOfDevices = upnp.discover(m_upnpTimeout, "urn:Belkin:device:controllee:1");
	for (const auto &address : listOfDevices) {
		if (m_stop)
			break;

		BelkinWemoSwitch::Ptr newDevice;
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

vector<BelkinWemoBulb::Ptr> BelkinWemoDeviceManager::seekBulbs()
{
	UPnP upnp;
	list<SocketAddress> listOfDevices;
	vector<BelkinWemoBulb::Ptr> devices;

	listOfDevices = upnp.discover(m_upnpTimeout, "urn:Belkin:device:bridge:1");
	for (const auto &address : listOfDevices) {
		if (m_stop)
			break;

		logger().debug("discovered a device at " + address.toString(),  __FILE__, __LINE__);

		BelkinWemoLink::Ptr link;
		try {
			link = BelkinWemoLink::buildDevice(address, m_httpTimeout);
		}
		catch (const TimeoutException& e) {
			logger().debug("found device has disconnected", __FILE__, __LINE__);
			continue;
		}

		logger().notice("discovered Belkin Wemo Link " + link->macAddress().toString(), __FILE__, __LINE__);

		/*
		 * Finds out if the link is already added.
		 * If the link already exists but has different IP address
		 * update the link.
		 */
		ScopedLockWithUnlock<FastMutex> linksLock(m_linksMutex);
		auto itLink = m_links.emplace(link->macAddress(), link);
		if (!itLink.second) {
			BelkinWemoLink::Ptr existingLink = itLink.first->second;
			ScopedLock<FastMutex> guard(existingLink->lock());

			existingLink->setAddress(link->address());
			link = existingLink;

			logger().information("updating address of Belkin Wemo Link " + link->macAddress().toString(),
				__FILE__, __LINE__);
		}
		linksLock.unlock();

		logger().notice("discovering Belkin Wemo Bulbs...", __FILE__, __LINE__);

		ScopedLock<FastMutex> guard(link->lock());
		list<BelkinWemoLink::BulbID> bulbIDs = link->requestDeviceList();

		logger().notice("discovered link with " + to_string(bulbIDs.size()) + " Belkin Wemo Bulbs", __FILE__, __LINE__);

		for (auto id : bulbIDs) {
			BelkinWemoBulb::Ptr newDevice = new BelkinWemoBulb(id, link);
			devices.push_back(newDevice);

			logger().information("discovered Belkin Wemo Bulb " + newDevice->deviceID().toString());
		}
	}

	return devices;
}

void BelkinWemoDeviceManager::processNewDevice(BelkinWemoDevice::Ptr newDevice)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	/*
	 * Finds out if the device is already added.
	 * If the device already exists but has different IP address
	 * update the device.
	 */
	auto it = m_devices.emplace(newDevice->deviceID(), newDevice);

	if (!it.second) {
		if (!newDevice.cast<BelkinWemoSwitch>().isNull()) {
			BelkinWemoSwitch::Ptr device = it.first->second.cast<BelkinWemoSwitch>();
			ScopedLock<FastMutex> guard(device->lock());

			device->setAddress(newDevice.cast<BelkinWemoSwitch>()->address());
		}

		return;
	}

	logger().debug("found device " + newDevice->deviceID().toString(),
		__FILE__, __LINE__);

	auto types = newDevice->moduleTypes();
	dispatch(
		new NewDeviceCommand(
			newDevice->deviceID(),
			BELKIN_WEMO_VENDOR,
			newDevice->name(),
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

		for (auto device : m_parent.seekBulbs()) {
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
