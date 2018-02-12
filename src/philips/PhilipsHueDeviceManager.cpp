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
#include "core/Answer.h"
#include "core/CommandDispatcher.h"
#include "credentials/PasswordCredentials.h"
#include "di/Injectable.h"
#include "model/DevicePrefix.h"
#include "net/UPnP.h"
#include "philips/PhilipsHueBulbInfo.h"
#include "philips/PhilipsHueBridgeInfo.h"
#include "philips/PhilipsHueDeviceManager.h"
#include "philips/PhilipsHueDimmableBulb.h"

#define PHILIPS_HUE_VENDOR "Philips Hue"

BEEEON_OBJECT_BEGIN(BeeeOn, PhilipsHueDeviceManager)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_REF("distributor", &PhilipsHueDeviceManager::setDistributor)
BEEEON_OBJECT_REF("commandDispatcher", &PhilipsHueDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_TIME("upnpTimeout", &PhilipsHueDeviceManager::setUPnPTimeout)
BEEEON_OBJECT_TIME("httpTimeout", &PhilipsHueDeviceManager::setHTTPTimeout)
BEEEON_OBJECT_TIME("refresh", &PhilipsHueDeviceManager::setRefresh)
BEEEON_OBJECT_REF("credentialsStorage", &PhilipsHueDeviceManager::setCredentialsStorage)
BEEEON_OBJECT_REF("cryptoConfig", &PhilipsHueDeviceManager::setCryptoConfig)
BEEEON_OBJECT_REF("eventsExecutor", &PhilipsHueDeviceManager::setEventsExecutor)
BEEEON_OBJECT_REF("listeners", &PhilipsHueDeviceManager::registerListener)
BEEEON_OBJECT_END(BeeeOn, PhilipsHueDeviceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Crypto;
using namespace Poco::Net;
using namespace std;

const Poco::Timespan PhilipsHueDeviceManager::SEARCH_DELAY = 45 * Timespan::SECONDS;

PhilipsHueDeviceManager::PhilipsHueDeviceManager():
	DeviceManager(DevicePrefix::PREFIX_PHILIPS_HUE),
	m_refresh(5 * Timespan::SECONDS),
	m_seeker(*this),
	m_httpTimeout(3 * Timespan::SECONDS),
	m_upnpTimeout(5 * Timespan::SECONDS)
{
}

void PhilipsHueDeviceManager::run()
{
	logger().information("starting Philips Hue device manager", __FILE__, __LINE__);

	try {
		m_pairedDevices = deviceList(-1);
	}
	catch (Exception& e) {
		logger().log(e, __FILE__, __LINE__);
	}

	if (m_pairedDevices.size() > 0)
		searchPairedDevices();

	while (!m_stop) {
		Timestamp now;

		eraseUnusedBridges();

		refreshPairedDevices();

		Timespan sleepTime = m_refresh - now.elapsed();
		if (sleepTime > 0)
			m_event.tryWait(sleepTime.totalMilliseconds());
	}

	logger().information("stopping Philips Hue device manager", __FILE__, __LINE__);

	m_seekerThread.join(m_upnpTimeout.totalMilliseconds());
	m_stop = false;
}

void PhilipsHueDeviceManager::stop()
{
	m_stop = true;
	m_event.set();
	m_seeker.stop();
	answerQueue().dispose();
}

void PhilipsHueDeviceManager::setRefresh(const Timespan &refresh)
{
	if (refresh.totalSeconds() <= 0)
		throw InvalidArgumentException("refresh time must at least a second");

	m_refresh = refresh;
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
	for (auto &id : m_pairedDevices) {
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
			logger().warning("device " + device->deviceID().toString() +
				" did not answer", __FILE__, __LINE__);
		}
	}
}

void PhilipsHueDeviceManager::searchPairedDevices()
{
	vector<PhilipsHueBulb::Ptr> bulbs = seekBulbs();

	ScopedLockWithUnlock<FastMutex> lockBulb(m_pairedMutex);
	for (auto device : bulbs) {
		auto it = m_pairedDevices.find(device->deviceID());
		if (it != m_pairedDevices.end())
			m_devices.emplace(device->deviceID(), device);
	}
	lockBulb.unlock();
}

void PhilipsHueDeviceManager::eraseUnusedBridges()
{
	try {
		ScopedLock<FastMutex> lock(m_bridgesMutex, m_refresh.totalMilliseconds());

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

bool PhilipsHueDeviceManager::accept(const Command::Ptr cmd)
{
	if (cmd->is<GatewayListenCommand>()) {
		return true;
	}
	else if (cmd->is<DeviceSetValueCommand>()) {
		return cmd->cast<DeviceSetValueCommand>().deviceID().prefix() ==
			DevicePrefix::PREFIX_PHILIPS_HUE;
	}
	else if (cmd->is<DeviceUnpairCommand>()) {
		return cmd->cast<DeviceUnpairCommand>().deviceID().prefix() ==
			DevicePrefix::PREFIX_PHILIPS_HUE;
	}
	else if (cmd->is<DeviceAcceptCommand>()) {
		return cmd->cast<DeviceAcceptCommand>().deviceID().prefix() ==
			DevicePrefix::PREFIX_PHILIPS_HUE;
	}

	return false;
}

void PhilipsHueDeviceManager::handle(Command::Ptr cmd, Answer::Ptr answer)
{
	if (cmd->is<GatewayListenCommand>()) {
		doListenCommand(cmd, answer);
	}
	else if (cmd->is<DeviceSetValueCommand>()) {
		DeviceSetValueCommand::Ptr cmdSet = cmd.cast<DeviceSetValueCommand>();

		Result::Ptr result = new Result(answer);

		if (modifyValue(cmdSet->deviceID(), cmdSet->moduleID(), cmdSet->value())) {
			result->setStatus(Result::Status::SUCCESS);

			SensorData data;
			data.setDeviceID(cmdSet->deviceID());
			data.insertValue({cmdSet->moduleID(), cmdSet->value()});
			ship(data);

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

void PhilipsHueDeviceManager::doListenCommand(const Command::Ptr cmd, const Answer::Ptr answer)
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

void PhilipsHueDeviceManager::doUnpairCommand(const Command::Ptr cmd, const Answer::Ptr answer)
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

void PhilipsHueDeviceManager::doDeviceAcceptCommand(const Command::Ptr cmd, const Answer::Ptr answer)
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

vector<PhilipsHueBulb::Ptr> PhilipsHueDeviceManager::seekBulbs()
{
	UPnP upnp;
	list<SocketAddress> listOfDevices;
	vector<PhilipsHueBulb::Ptr> devices;

	listOfDevices = upnp.discover(m_upnpTimeout, "urn:schemas-upnp-org:device:basic:1");
	for (const auto &address : listOfDevices) {
		if (m_stop)
			break;

		logger().debug("discovered a device at " + address.toString(),  __FILE__, __LINE__);

		PhilipsHueBridge::Ptr bridge;
		try {
			bridge = PhilipsHueBridge::buildDevice(address, m_httpTimeout);
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
					bulb.second.first, bulb.second.second, bridge);
				devices.push_back(newDevice);

				logger().information("discovered Philips Hue Bulb " + newDevice->deviceID().toString(),
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
	auto it = m_devices.emplace(newDevice->deviceID(), newDevice);

	if (!it.second)
		return;

	logger().debug("found device " + newDevice->deviceID().toString(),
		__FILE__, __LINE__);

	auto types = newDevice->moduleTypes();
	dispatch(
		new NewDeviceCommand(
			newDevice->deviceID(),
			PHILIPS_HUE_VENDOR,
			newDevice->name(),
			types,
			m_refresh));

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

PhilipsHueDeviceManager::PhilipsHueSeeker::PhilipsHueSeeker(PhilipsHueDeviceManager& parent) :
	m_parent(parent),
	m_stop(false)
{
}

void PhilipsHueDeviceManager::PhilipsHueSeeker::setDuration(const Poco::Timespan& duration)
{
	m_duration = duration;
}

void PhilipsHueDeviceManager::PhilipsHueSeeker::run()
{
	Timestamp now;

	while (now.elapsed() < m_duration.totalMicroseconds()) {
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

void PhilipsHueDeviceManager::PhilipsHueSeeker::stop()
{
	m_stop = true;
}
