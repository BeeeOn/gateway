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
#include "util/BlockingAsyncWork.h"

#define BELKIN_WEMO_VENDOR "Belkin WeMo"

BEEEON_OBJECT_BEGIN(BeeeOn, BelkinWemoDeviceManager)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(DeviceStatusHandler)
BEEEON_OBJECT_PROPERTY("deviceCache", &BelkinWemoDeviceManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("devicePoller", &BelkinWemoDeviceManager::setDevicePoller)
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
	m_refresh(RefreshTime::fromSeconds(5)),
	m_httpTimeout(3 * Timespan::SECONDS),
	m_upnpTimeout(5 * Timespan::SECONDS)
{
}

void BelkinWemoDeviceManager::setDevicePoller(DevicePoller::Ptr poller)
{
	m_pollingKeeper.setDevicePoller(poller);
}

void BelkinWemoDeviceManager::run()
{
	logger().information("starting Belkin WeMo device manager", __FILE__, __LINE__);

	StopControl::Run run(m_stopControl);

	while (run) {
		searchPairedDevices();
		eraseUnusedLinks();

		for (auto pair : m_devices) {
			if (deviceCache()->paired(pair.second->id()))
				m_pollingKeeper.schedule(pair.second);
			else
				m_pollingKeeper.cancel(pair.second->id());
		}

		run.waitStoppable(m_refresh);
	}

	m_pollingKeeper.cancelAll();
	logger().information("stopping Belkin WeMo device manager", __FILE__, __LINE__);
}

void BelkinWemoDeviceManager::stop()
{
	DeviceManager::stop();
	answerQueue().dispose();
}

void BelkinWemoDeviceManager::setRefresh(const Timespan &refresh)
{
	if (refresh.totalSeconds() <= 0)
		throw InvalidArgumentException("refresh time must at least a second");

	m_refresh = RefreshTime::fromSeconds(refresh.totalSeconds());
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

void BelkinWemoDeviceManager::searchPairedDevices()
{
	set<DeviceID> pairedDevices;
	ScopedLockWithUnlock<FastMutex> guard(m_pairedMutex);
	for (auto &id : deviceCache()->paired(prefix())) {
		auto it = m_devices.find(id);
		if (it == m_devices.end())
			pairedDevices.insert(id);
	}
	guard.unlock();

	if (pairedDevices.size() <= 0)
		return;

	logger().information("discovering of paired devices...", __FILE__, __LINE__);

	vector<BelkinWemoSwitch::Ptr> switches = seekSwitches(m_stopControl);
	vector<BelkinWemoBulb::Ptr> bulbs = seekBulbs(m_stopControl);
	vector<BelkinWemoDimmer::Ptr> dimmers = seekDimmers(m_stopControl);

	vector<BelkinWemoDevice::Ptr> foundDevices;
	foundDevices.insert(foundDevices.end(), switches.begin(), switches.end());
	foundDevices.insert(foundDevices.end(), bulbs.begin(), bulbs.end());
	foundDevices.insert(foundDevices.end(), dimmers.begin(), dimmers.end());

	ScopedLock<FastMutex> lock(m_pairedMutex);
	for (const auto &device : foundDevices) {
		if (pairedDevices.find(device->id()) == pairedDevices.end())
			continue;

		m_devices.emplace(device->id(), device);

		logger().information("found " + device->name() + " " + device->id().toString(),
			__FILE__, __LINE__);
	}
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
	else {
		DeviceManager::handleGeneric(cmd, result);
	}
}

AsyncWork<>::Ptr BelkinWemoDeviceManager::startDiscovery(const Timespan &timeout)
{
	BelkinWemoSeeker::Ptr seeker = new BelkinWemoSeeker(*this, timeout);
	seeker->start();
	return seeker;
}

AsyncWork<set<DeviceID>>::Ptr BelkinWemoDeviceManager::startUnpair(
		const DeviceID &id,
		const Timespan &)
{
	auto work = BlockingAsyncWork<set<DeviceID>>::instance();

	FastMutex::ScopedLock lock(m_pairedMutex);

	if (!deviceCache()->paired(id)) {
		logger().warning("unpairing device that is not paired: " + id.toString(),
			__FILE__, __LINE__);
	}
	else {
		deviceCache()->markUnpaired(id);
		m_pollingKeeper.cancel(id);

		auto itDevice = m_devices.find(id);
		if (itDevice != m_devices.end())
			m_devices.erase(id);

		work->setResult({id});
	}

	return work;
}

void BelkinWemoDeviceManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	auto it = m_devices.find(cmd->deviceID());
	if (it == m_devices.end())
		throw NotFoundException("accept: " + cmd->deviceID().toString());

	DeviceManager::handleAccept(cmd);
	m_pollingKeeper.schedule(it->second);
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
			newDevice = new BelkinWemoSwitch(address, m_httpTimeout, m_refresh);
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
			link = new BelkinWemoLink(address, m_httpTimeout);
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
			BelkinWemoBulb::Ptr newDevice = new BelkinWemoBulb(id, link, m_refresh);
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
			newDevice = new BelkinWemoDimmer(address, m_httpTimeout, m_refresh);
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

	auto builder = DeviceDescription::Builder();

	const auto standalone = newDevice.cast<BelkinWemoStandaloneDevice>();
	if (!standalone.isNull())
		builder.ipAddress(standalone->address().host());

	const auto bulb = newDevice.cast<BelkinWemoBulb>();
	if (!bulb.isNull()) {
		builder.ipAddress(bulb->link()->address().host());
		builder.macAddress(bulb->link()->macAddress());
	}

	const auto description = builder
		.id(newDevice->deviceID())
		.type(BELKIN_WEMO_VENDOR, newDevice->name())
		.modules(newDevice->moduleTypes())
		.refreshTime(m_refresh)
		.build();

	dispatch(new NewDeviceCommand(description));
}

BelkinWemoDeviceManager::BelkinWemoSeeker::BelkinWemoSeeker(BelkinWemoDeviceManager& parent, const Timespan& duration) :
	AbstractSeeker(duration),
	m_parent(parent)
{
}

void BelkinWemoDeviceManager::BelkinWemoSeeker::seekLoop(StopControl &control)
{
	StopControl::Run run(control);

	while (remaining() > 0) {
		for (auto device : m_parent.seekSwitches(control)) {
			if (!run)
				break;

			m_parent.processNewDevice(device);
		}

		if (!run)
			break;

		for (auto device : m_parent.seekBulbs(control)) {
			if (!run)
				break;

			m_parent.processNewDevice(device);
		}

		if (!run)
			break;

		for (auto device : m_parent.seekDimmers(control)) {
			if (!run)
				break;

			m_parent.processNewDevice(device);
		}

		if (!run)
			break;
	}
}
