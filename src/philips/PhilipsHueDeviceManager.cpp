#include <Poco/ScopedLock.h>
#include <Poco/Timestamp.h>
#include <Poco/Crypto/Cipher.h>
#include <Poco/Crypto/CipherFactory.h>
#include <Poco/Crypto/CipherKey.h>
#include <Poco/Net/SocketAddress.h>

#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "credentials/PasswordCredentials.h"
#include "di/Injectable.h"
#include "model/DevicePrefix.h"
#include "net/UPnP.h"
#include "philips/PhilipsHueBulbInfo.h"
#include "philips/PhilipsHueBridgeInfo.h"
#include "philips/PhilipsHueDeviceManager.h"
#include "philips/PhilipsHueDimmableBulb.h"
#include "util/BlockingAsyncWork.h"

#define PHILIPS_HUE_VENDOR "Philips Hue"

BEEEON_OBJECT_BEGIN(BeeeOn, PhilipsHueDeviceManager)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(DeviceStatusHandler)
BEEEON_OBJECT_PROPERTY("deviceCache", &PhilipsHueDeviceManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("distributor", &PhilipsHueDeviceManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &PhilipsHueDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("upnpTimeout", &PhilipsHueDeviceManager::setUPnPTimeout)
BEEEON_OBJECT_PROPERTY("httpTimeout", &PhilipsHueDeviceManager::setHTTPTimeout)
BEEEON_OBJECT_PROPERTY("refresh", &PhilipsHueDeviceManager::setRefresh)
BEEEON_OBJECT_PROPERTY("credentialsStorage", &PhilipsHueDeviceManager::setCredentialsStorage)
BEEEON_OBJECT_PROPERTY("cryptoConfig", &PhilipsHueDeviceManager::setCryptoConfig)
BEEEON_OBJECT_PROPERTY("eventsExecutor", &PhilipsHueDeviceManager::setEventsExecutor)
BEEEON_OBJECT_PROPERTY("listeners", &PhilipsHueDeviceManager::registerListener)
BEEEON_OBJECT_END(BeeeOn, PhilipsHueDeviceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Crypto;
using namespace Poco::Net;
using namespace std;

const Poco::Timespan PhilipsHueDeviceManager::SEARCH_DELAY = 45 * Timespan::SECONDS;

PhilipsHueDeviceManager::PhilipsHueDeviceManager():
	DeviceManager(DevicePrefix::PREFIX_PHILIPS_HUE, {
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

void PhilipsHueDeviceManager::run()
{
	logger().information("starting Philips Hue device manager", __FILE__, __LINE__);

	set<DeviceID> paired = waitRemoteStatus(-1);

	if (paired.size() > 0)
		searchPairedDevices();

	StopControl::Run run(m_stopControl);

	while (run) {
		Timestamp now;

		eraseUnusedBridges();

		refreshPairedDevices();

		Timespan sleepTime = m_refresh.time() - now.elapsed();
		if (sleepTime > 0)
			run.waitStoppable(sleepTime);
	}

	logger().information("stopping Philips Hue device manager", __FILE__, __LINE__);
}

void PhilipsHueDeviceManager::stop()
{
	DeviceManager::stop();
	answerQueue().dispose();
}

void PhilipsHueDeviceManager::setRefresh(const Timespan &refresh)
{
	if (refresh.totalSeconds() <= 0)
		throw InvalidArgumentException("refresh time must at least a second");

	m_refresh = RefreshTime::fromSeconds(refresh.totalSeconds());
}

void PhilipsHueDeviceManager::setUPnPTimeout(const Timespan &timeout)
{
	if (timeout.totalSeconds() <= 0)
		throw InvalidArgumentException("UPnP timeout time must be at least a second");

	m_upnpTimeout = timeout;
}

void PhilipsHueDeviceManager::setHTTPTimeout(const Timespan &timeout)
{
	if (timeout.totalSeconds() <= 0)
		throw InvalidArgumentException("http timeout time must be at least a second");

	m_httpTimeout = timeout;
}

void PhilipsHueDeviceManager::setCredentialsStorage(
	SharedPtr<FileCredentialsStorage> storage)
{
	m_credentialsStorage = storage;
}

void PhilipsHueDeviceManager::setCryptoConfig(SharedPtr<CryptoConfig> config)
{
	m_cryptoConfig = config;
}

void PhilipsHueDeviceManager::setEventsExecutor(AsyncExecutor::Ptr executor)
{
	m_eventSource.setAsyncExecutor(executor);
}

void PhilipsHueDeviceManager::registerListener(PhilipsHueListener::Ptr listener)
{
	m_eventSource.addListener(listener);
}

void PhilipsHueDeviceManager::refreshPairedDevices()
{
	vector<PhilipsHueBulb::Ptr> devices;

	ScopedLockWithUnlock<FastMutex> lock(m_pairedMutex);
	for (auto &id : deviceCache()->paired(prefix())) {
		auto it = m_devices.find(id);
		if (it == m_devices.end()) {
			logger().warning("no such device: " + id.toString(),
				__FILE__, __LINE__);
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
			logger().warning("device " + device->id().toString() +
				" did not answer", __FILE__, __LINE__);
		}
	}
}

void PhilipsHueDeviceManager::searchPairedDevices()
{
	vector<PhilipsHueBulb::Ptr> bulbs = seekBulbs(m_stopControl);

	ScopedLockWithUnlock<FastMutex> lockBulb(m_pairedMutex);
	for (auto device : bulbs) {
		if (deviceCache()->paired(device->id()))
			m_devices.emplace(device->id(), device);
	}
	lockBulb.unlock();
}

void PhilipsHueDeviceManager::eraseUnusedBridges()
{
	try {
		ScopedLock<FastMutex> lock(m_bridgesMutex, m_refresh.time().totalMilliseconds());

		vector<PhilipsHueBridge::Ptr> bridges;

		for (auto bridge : m_bridges) {
			ScopedLock<FastMutex> guard(bridge.second->lock());

			if (bridge.second->countOfBulbs() == 0)
				bridges.push_back(bridge.second);
		}

		for (auto bridge : bridges) {
			logger().debug("erase Philips Hue Bridge " +
				bridge->macAddress().toString(), __FILE__, __LINE__);

			m_credentialsStorage->remove(
				DeviceID(DevicePrefix::PREFIX_PHILIPS_HUE, bridge->macAddress()));
			m_bridges.erase(bridge->macAddress());
		}
	}
	catch (const TimeoutException& e) {
		// The lock is held by listen thread which makes the authorization of bridge.
		// It can last several tens of seconds.
		return;
	}
}

void PhilipsHueDeviceManager::handleGeneric(const Command::Ptr cmd, Result::Ptr result)
{
	if (cmd->is<DeviceSetValueCommand>()) {
		doSetValueCommand(cmd);
	}
	else {
		DeviceManager::handleGeneric(cmd, result);
	}
}

AsyncWork<>::Ptr PhilipsHueDeviceManager::startDiscovery(const Timespan &timeout)
{
	PhilipsHueSeeker::Ptr seeker = new PhilipsHueSeeker(*this, timeout);
	seeker->start();

	return seeker;
}

AsyncWork<set<DeviceID>>::Ptr PhilipsHueDeviceManager::startUnpair(
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

		auto itDevice = m_devices.find(id);
		if (itDevice != m_devices.end())
			m_devices.erase(id);

		work->setResult({id});
	}

	return work;
}

void PhilipsHueDeviceManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	auto it = m_devices.find(cmd->deviceID());
	if (it == m_devices.end())
		throw NotFoundException("accept: " + cmd->deviceID().toString());

	DeviceManager::handleAccept(cmd);
}

void PhilipsHueDeviceManager::doSetValueCommand(const Command::Ptr cmd)
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
	BEEEON_CATCH_CHAIN(logger())
}

bool PhilipsHueDeviceManager::modifyValue(const DeviceID& deviceID,
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

vector<PhilipsHueBulb::Ptr> PhilipsHueDeviceManager::seekBulbs(const StopControl& stop)
{
	UPnP upnp;
	list<SocketAddress> listOfDevices;
	vector<PhilipsHueBulb::Ptr> devices;

	listOfDevices = upnp.discover(m_upnpTimeout, "urn:schemas-upnp-org:device:basic:1");
	for (const auto &address : listOfDevices) {
		if (stop.shouldStop())
			break;

		logger().debug("discovered a device at " + address.toString(),  __FILE__, __LINE__);

		PhilipsHueBridge::Ptr bridge;
		try {
			bridge = new PhilipsHueBridge(address, m_httpTimeout);
		}
		catch (const TimeoutException& e) {
			logger().debug("found device has disconnected", __FILE__, __LINE__);
			continue;
		}

		logger().notice("discovered Philips Hue Bridge " + bridge->macAddress().toString(),
			__FILE__, __LINE__);

		/*
		 * Finds out if the bridge is already added.
		 * If the bridge already exists but has different IP address
		 * update the bridge.
		 */
		ScopedLock<FastMutex> bridgesLock(m_bridgesMutex);
		auto itBridge = m_bridges.emplace(bridge->macAddress(), bridge);

		if (!itBridge.second) {
			PhilipsHueBridge::Ptr existingBridge = itBridge.first->second;
			ScopedLock<FastMutex> guard(existingBridge->lock());

			existingBridge->setAddress(bridge->address());
			bridge = existingBridge;

			logger().information("updating address of Philips Hue Bridge " + bridge->macAddress().toString(),
				__FILE__, __LINE__);
		}
		else {
			try {
				authorizationOfBridge(bridge);
			}
			catch (const TimeoutException& e) {
				logger().debug("authorization of gateway to the Philips bridge exceeded timeout",
					__FILE__, __LINE__);
				continue;
			}
			catch (const DataFormatException& e) {
				logger().debug("authorization of gateway to the Philips bridge failed due to bad format of username",
					__FILE__, __LINE__);
				continue;
			}

			fireBridgeStatistics(bridge);
		}

		logger().notice("discovering Philips Hue Bulbs...", __FILE__, __LINE__);

		list<pair<string, pair<uint32_t, PhilipsHueBridge::BulbID>>> bulbs;
		try {
			ScopedLockWithUnlock<FastMutex> guardSearch(bridge->lock());
			bridge->requestSearchNewDevices();
			guardSearch.unlock();

			Thread::sleep(SEARCH_DELAY.totalMilliseconds());

			ScopedLockWithUnlock<FastMutex> guardDevices(bridge->lock());
			bulbs = bridge->requestDeviceList();
			guardDevices.unlock();
		}
		catch (const TimeoutException& e) {
			logger().debug("bridge has disconnected", __FILE__, __LINE__);
			continue;
		}

		logger().notice("discovered bridge with " + to_string(bulbs.size()) + " Philips Hue Bulbs",
			__FILE__, __LINE__);

		for (auto bulb : bulbs) {
			if (bulb.first == "Dimmable light") {
				PhilipsHueDimmableBulb::Ptr newDevice = new PhilipsHueDimmableBulb(
					bulb.second.first, bulb.second.second, bridge, m_refresh);
				devices.push_back(newDevice);

				logger().information("discovered Philips Hue Bulb " + newDevice->id().toString(),
					__FILE__, __LINE__);
			}
			else {
				logger().debug("unsupported bulb " + bulb.first, __FILE__, __LINE__);
			}
		}
	}

	return devices;
}

void PhilipsHueDeviceManager::authorizationOfBridge(PhilipsHueBridge::Ptr bridge)
{
	SharedPtr<Credentials> credential = m_credentialsStorage->find(
		DeviceID(DevicePrefix::PREFIX_PHILIPS_HUE, bridge->macAddress()));

	if (!credential.isNull()) {
		CipherKey key = m_cryptoConfig->createKey(credential->params());

		if (!credential.cast<PasswordCredentials>().isNull()) {
			ScopedLock<FastMutex> guard(bridge->lock());

			SharedPtr<PasswordCredentials> password = credential.cast<PasswordCredentials>();
			bridge->setCredentials(password, m_cryptoConfig);
		}
	}
	else {
		ScopedLockWithUnlock<FastMutex> guard(bridge->lock());
		string username = bridge->authorize();

		CipherFactory &cryptoFactory = CipherFactory::defaultFactory();
		CryptoParams cryptoParams = m_cryptoConfig->deriveParams();
		Cipher *cipher = cryptoFactory.createCipher(m_cryptoConfig->createKey(cryptoParams));

		SharedPtr<PasswordCredentials> password = new PasswordCredentials;
		password->setUsername(username, cipher);
		password->setPassword("", cipher);
		password->setParams(cryptoParams);

		m_credentialsStorage->insertOrUpdate(
			DeviceID(DevicePrefix::PREFIX_PHILIPS_HUE, bridge->macAddress()), password);

		bridge->setCredentials(password, m_cryptoConfig);
	}
}

void PhilipsHueDeviceManager::processNewDevice(PhilipsHueBulb::Ptr newDevice)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	/*
	 * Finds out if the device is already added.
	 */
	auto it = m_devices.emplace(newDevice->id(), newDevice);

	if (!it.second)
		return;

	logger().debug("found device " + newDevice->id().toString(),
		__FILE__, __LINE__);

	const auto description = DeviceDescription::Builder()
		.id(newDevice->id())
		.type(PHILIPS_HUE_VENDOR, newDevice->name())
		.modules(newDevice->moduleTypes())
		.refreshTime(m_refresh)
		.build();

	dispatch(new NewDeviceCommand(description));

	fireBulbStatistics(newDevice);
}

void PhilipsHueDeviceManager::fireBridgeStatistics(PhilipsHueBridge::Ptr bridge)
{
	try {
		PhilipsHueBridgeInfo info = bridge->info();
		m_eventSource.fireEvent(info, &PhilipsHueListener::onBridgeStats);
	}
	catch (Exception& e) {
		logger().debug("failed to obtain bridge info");
		logger().log(e, __FILE__, __LINE__);
	}
}

void PhilipsHueDeviceManager::fireBulbStatistics(PhilipsHueBulb::Ptr bulb)
{
	try {
		PhilipsHueBulbInfo info = bulb->info();
		m_eventSource.fireEvent(info, &PhilipsHueListener::onBulbStats);
	}
	catch (Exception& e) {
		logger().debug("failed to obtain bulb info");
		logger().log(e, __FILE__, __LINE__);
	}
}

PhilipsHueDeviceManager::PhilipsHueSeeker::PhilipsHueSeeker(PhilipsHueDeviceManager& parent, const Timespan &duration) :
	AbstractSeeker(duration),
	m_parent(parent)
{
}

void PhilipsHueDeviceManager::PhilipsHueSeeker::seekLoop(StopControl &control)
{
	StopControl::Run run(control);

	while (remaining() > 0) {
		for (auto device : m_parent.seekBulbs(control)) {
			if (!run)
				break;

			m_parent.processNewDevice(device);
		}

		if (!run)
			break;
	}
}
