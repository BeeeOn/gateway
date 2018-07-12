#include <Poco/Net/SocketAddress.h>
#include <Poco/ScopedLock.h>
#include <Poco/Timestamp.h>

#include "belkin/BelkinWemoDeviceManager.h"
#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "model/DevicePrefix.h"
#include "net/UPnP.h"

#define BELKIN_WEMO_VENDOR "Belkin WeMo"

BEEEON_OBJECT_BEGIN(BeeeOn, BelkinWemoDeviceManager)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_PROPERTY("deviceCache", &BelkinWemoDeviceManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("distributor", &BelkinWemoDeviceManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &BelkinWemoDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("upnpTimeout", &BelkinWemoDeviceManager::setUPnPTimeout)
BEEEON_OBJECT_PROPERTY("httpTimeout", &BelkinWemoDeviceManager::setHTTPTimeout)
BEEEON_OBJECT_PROPERTY("refresh", &BelkinWemoDeviceManager::setRefresh)
BEEEON_OBJECT_END(BeeeOn, BelkinWemoDeviceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Net;
using namespace std;

BelkinWemoDeviceManager::BelkinWemoDeviceManager():
	DeviceManager(DevicePrefix::PREFIX_BELKIN_WEMO, {
		typeid(GatewayListenCommand),
		typeid(DeviceAcceptCommand),
		typeid(DeviceUnpairCommand),
		typeid(DeviceSetValueCommand),
	}),
	m_refresh(5 * Timespan::SECONDS),
	m_seeker(new BelkinWemoSeeker(*this)),
	m_httpTimeout(3 * Timespan::SECONDS),
	m_upnpTimeout(5 * Timespan::SECONDS)
{
}

void BelkinWemoDeviceManager::run()
{
	logger().information("starting Belkin WeMo device manager", __FILE__, __LINE__);

	set<DeviceID> paired;
	paired = deviceList(-1);

	for (const auto &id : paired)
		deviceCache()->markPaired(id);

	if (paired.size() > 0)
		searchPairedDevices();

	StopControl::Run run(m_stopControl);

	while (run) {
		Timestamp now;

		eraseUnusedLinks();

		refreshPairedDevices();

		Timespan sleepTime = m_refresh - now.elapsed();
		if (sleepTime > 0)
			run.waitStoppable(sleepTime);
	}

	logger().information("stopping Belkin WeMo device manager", __FILE__, __LINE__);
}

void BelkinWemoDeviceManager::stop()
{
	DeviceManager::stop();
	m_seeker->stop();
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
	for (auto &id : deviceCache()->paired(prefix())) {
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
	vector<BelkinWemoSwitch::Ptr> switches = seekSwitches(m_stopControl);

	ScopedLockWithUnlock<FastMutex> lockSwitch(m_pairedMutex);
	for (auto device : switches) {
		if (deviceCache()->paired(device->deviceID()))
			m_devices.emplace(device->deviceID(), device);
	}
	lockSwitch.unlock();

	vector<BelkinWemoBulb::Ptr> bulbs = seekBulbs(m_stopControl);

	ScopedLockWithUnlock<FastMutex> lockBulb(m_pairedMutex);
	for (auto device : bulbs) {
		if (deviceCache()->paired(device->deviceID()))
			m_devices.emplace(device->deviceID(), device);
	}
	lockBulb.unlock();

	vector<BelkinWemoDimmer::Ptr> dimmers = seekDimmers(m_stopControl);

	ScopedLockWithUnlock<FastMutex> lockDimmer(m_pairedMutex);
	for (auto device : dimmers) {
		if (deviceCache()->paired(device->deviceID()))
			m_devices.emplace(device->deviceID(), device);
	}
	lockDimmer.unlock();
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

void BelkinWemoDeviceManager::handleGeneric(const Command::Ptr cmd, Result::Ptr result)
{
	if (cmd->is<DeviceSetValueCommand>()) {
		doSetValueCommand(cmd);
	}
	else if (cmd->is<DeviceUnpairCommand>()) {
		doUnpairCommand(cmd);
	}
	else {
		DeviceManager::handleGeneric(cmd, result);
	}
}

AsyncWork<>::Ptr BelkinWemoDeviceManager::startDiscovery(const Timespan &timeout)
{
	m_seeker->startSeeking(timeout);

	return m_seeker;
}

void BelkinWemoDeviceManager::doUnpairCommand(const Command::Ptr cmd)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	DeviceUnpairCommand::Ptr cmdUnpair = cmd.cast<DeviceUnpairCommand>();

	if (!deviceCache()->paired(cmdUnpair->deviceID())) {
		logger().warning("unpairing device that is not paired: " + cmdUnpair->deviceID().toString(),
			__FILE__, __LINE__);
	}
	else {
		deviceCache()->markUnpaired(cmdUnpair->deviceID());

		auto itDevice = m_devices.find(cmdUnpair->deviceID());
		if (itDevice != m_devices.end())
			m_devices.erase(cmdUnpair->deviceID());
	}
}

void BelkinWemoDeviceManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	auto it = m_devices.find(cmd->deviceID());
	if (it == m_devices.end())
		throw NotFoundException("accept: " + cmd->deviceID().toString());

	DeviceManager::handleAccept(cmd);
}

void BelkinWemoDeviceManager::doSetValueCommand(const Command::Ptr cmd)
{
	DeviceSetValueCommand::Ptr cmdSet = cmd.cast<DeviceSetValueCommand>();

	if (!modifyValue(cmdSet->deviceID(), cmdSet->moduleID(), cmdSet->value())) {
		throw IllegalStateException(
				"set-value: " + cmdSet->deviceID().toString());
	}

	logger().debug("success to change state of device " + cmdSet->deviceID().toString(),
		__FILE__, __LINE__);

	try {
		SensorData data;
		data.setDeviceID(cmdSet->deviceID());
		data.insertValue({cmdSet->moduleID(), cmdSet->value()});
		ship(data);
	}
	BEEEON_CATCH_CHAIN(logger());
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

vector<BelkinWemoSwitch::Ptr> BelkinWemoDeviceManager::seekSwitches(const StopControl& stop)
{
	UPnP upnp;
	list<SocketAddress> listOfDevices;
	vector<BelkinWemoSwitch::Ptr> devices;

	listOfDevices = upnp.discover(m_upnpTimeout, "urn:Belkin:device:controllee:1");
	for (const auto &address : listOfDevices) {
		if (stop.shouldStop())
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

vector<BelkinWemoBulb::Ptr> BelkinWemoDeviceManager::seekBulbs(const StopControl& stop)
{
	UPnP upnp;
	list<SocketAddress> listOfDevices;
	vector<BelkinWemoBulb::Ptr> devices;

	listOfDevices = upnp.discover(m_upnpTimeout, "urn:Belkin:device:bridge:1");
	for (const auto &address : listOfDevices) {
		if (stop.shouldStop())
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

vector<BelkinWemoDimmer::Ptr> BelkinWemoDeviceManager::seekDimmers(const StopControl& stop)
{
	UPnP upnp;
	list<SocketAddress> listOfDevices;
	vector<BelkinWemoDimmer::Ptr> devices;

	listOfDevices = upnp.discover(m_upnpTimeout, "urn:Belkin:device:dimmer:1");
	for (const auto &address : listOfDevices) {
		if (stop.shouldStop())
			break;

		BelkinWemoDimmer::Ptr newDevice;
		try {
			newDevice = BelkinWemoDimmer::buildDevice(address, m_httpTimeout);
		}
		catch (const TimeoutException& e) {
			logger().debug("found device has disconnected", __FILE__, __LINE__);
			continue;
		}

		devices.push_back(newDevice);
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
		else if (!newDevice.cast<BelkinWemoDimmer>().isNull()) {
			BelkinWemoDimmer::Ptr device = it.first->second.cast<BelkinWemoDimmer>();
			ScopedLock<FastMutex> guard(device->lock());

			device->setAddress(newDevice.cast<BelkinWemoDimmer>()->address());
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
			types,
			m_refresh));
}

BelkinWemoDeviceManager::BelkinWemoSeeker::BelkinWemoSeeker(BelkinWemoDeviceManager& parent) :
	m_parent(parent)
{
}

void BelkinWemoDeviceManager::BelkinWemoSeeker::startSeeking(const Timespan& duration)
{
	if (!m_seekerThread.isRunning()) {
		m_seekerThread.start(*this);
	}
	else {
		m_parent.logger().debug("listen seems to be running already, dropping listen command", __FILE__, __LINE__);
	}

	m_duration = duration;
}

void BelkinWemoDeviceManager::BelkinWemoSeeker::run()
{
	Timestamp now;
	StopControl::Run run(m_stopControl);

	while (now.elapsed() < m_duration.totalMicroseconds()) {
		for (auto device : m_parent.seekSwitches(m_stopControl)) {
			if (!run)
				break;

			m_parent.processNewDevice(device);
		}

		if (!run)
			break;

		for (auto device : m_parent.seekBulbs(m_stopControl)) {
			if (!run)
				break;

			m_parent.processNewDevice(device);
		}

		if (!run)
			break;

		for (auto device : m_parent.seekDimmers(m_stopControl)) {
			if (!run)
				break;

			m_parent.processNewDevice(device);
		}

		if (!run)
			break;
	}
}

void BelkinWemoDeviceManager::BelkinWemoSeeker::stop()
{
	m_stopControl.requestStop();
	m_seekerThread.join(m_parent.m_upnpTimeout.totalMilliseconds());
}

bool BelkinWemoDeviceManager::BelkinWemoSeeker::tryJoin(const Timespan &timeout)
{
	return m_seekerThread.tryJoin(timeout.totalMilliseconds());
}

void BelkinWemoDeviceManager::BelkinWemoSeeker::cancel()
{
	stop();
}
