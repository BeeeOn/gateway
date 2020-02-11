#include <Poco/RegularExpression.h>

#include "commands/DeviceUnpairCommand.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceAcceptCommand.h"
#include "commands/NewDeviceCommand.h"
#include "commands/GatewayListenCommand.h"
#include "conrad/ConradDeviceManager.h"
#include "conrad/ConradEvent.h"
#include "conrad/PowerMeterSwitch.h"
#include "conrad/RadiatorThermostat.h"
#include "conrad/WirelessShutterContact.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "loop/StopControl.h"
#include "model/SensorData.h"
#include "model/DevicePrefix.h"

#define CONRAD_VENDOR "Conrad"

BEEEON_OBJECT_BEGIN(BeeeOn, ConradDeviceManager)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(DeviceStatusHandler)
BEEEON_OBJECT_PROPERTY("deviceCache", &ConradDeviceManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("distributor", &ConradDeviceManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &ConradDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("fhemClient", &ConradDeviceManager::setFHEMClient)
BEEEON_OBJECT_PROPERTY("eventsExecutor", &ConradDeviceManager::setEventsExecutor)
BEEEON_OBJECT_PROPERTY("listeners", &ConradDeviceManager::registerListener)
BEEEON_OBJECT_END(BeeeOn, ConradDeviceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::JSON;
using namespace std;

ConradDeviceManager::ConradDeviceManager() :
	DeviceManager(DevicePrefix::PREFIX_CONRAD, {
		typeid(GatewayListenCommand),
		typeid(DeviceAcceptCommand),
		typeid(DeviceUnpairCommand),
		typeid(DeviceSetValueCommand),
	})
{
}

void ConradDeviceManager::setFHEMClient(FHEMClient::Ptr fhemClient)
{
	m_fhemClient = fhemClient;
}

void ConradDeviceManager::setEventsExecutor(AsyncExecutor::Ptr executor)
{
	m_eventSource.setAsyncExecutor(executor);
}

void ConradDeviceManager::registerListener(ConradListener::Ptr listener)
{
	m_eventSource.addListener(listener);
}

void ConradDeviceManager::run()
{
	logger().information("starting Conrad device manager", __FILE__, __LINE__);

	StopControl::Run run(m_stopControl);
	while (run) {
		Object::Ptr event = m_fhemClient->receive(-1);

		try {
			processEvent(event);
		}
		catch (const Exception& e) {
			logger().log(e, __FILE__, __LINE__);
		}
	}

	logger().information("stopping Conrad device manager", __FILE__, __LINE__);
}

void ConradDeviceManager::processEvent(const Object::Ptr event)
{
	if (logger().trace()) {
		ostringstream jsonStream;
		event->stringify(jsonStream, 2);

		logger().dump(
			"received event of size " +
			to_string(jsonStream.str().size()) + " B",
			jsonStream.str().c_str(),
			jsonStream.str().size(),
			Message::PRIO_TRACE);
	}

	if (!event->has("dev"))
		throw IllegalStateException("event does not contain 'dev' element");

	string devId = event->getValue<string>("dev");
	RegularExpression reDeviceId("HM_([a-fA-F0-9]+)");
	RegularExpression::MatchVec matches;
	if (reDeviceId.match(event->getValue<string>("dev"), 0, matches) == 0)
		throw IllegalStateException("event contains 'dev' element with wrong format");

	devId = devId.substr(matches[1].offset, matches[1].length);
	uint32_t id = NumberParser::parseHex(devId);
	DeviceID deviceID = DeviceID(DevicePrefix::PREFIX_CONRAD, id);

	if (logger().debug()) {
		logger().debug("Event " + event->getValue<string>("event") +
			" from " + deviceID.toString(), __FILE__, __LINE__);
	}

	fireMessage(deviceID, event);

	ScopedLock<FastMutex> lock(m_devicesMutex);

	string eventType = event->getValue<string>("event");
	if (eventType == "new_device") {
		createNewDeviceUnlocked(deviceID, event->getValue<string>("type"));
	}
	else if (eventType == "message") {
		processMessageEvent(deviceID, event);
	}
	else if (eventType == "rcv_cnt" || eventType == "snd_cnt") {
		return;
	}
	else {
		throw IllegalStateException("unknown event");
	}
}

void ConradDeviceManager::processMessageEvent(
		const DeviceID& deviceID,
		const Object::Ptr event)
{
	auto it = m_devices.find(deviceID);
	if (it == m_devices.end())
		createNewDeviceUnlocked(deviceID, event->getValue<string>("type"));

	if (!deviceCache()->paired(deviceID))
		return;

	SensorData data = it->second->parseMessage(event);
	try {
		if (data.size() > 0)
			ship(data);
	}
	catch (const Exception& e) {
		logger().log(e, __FILE__, __LINE__);
	}
}

void ConradDeviceManager::createNewDeviceUnlocked(
		const DeviceID &deviceID,
		const string &type)
{
	ConradDevice::Ptr newDevice;

	if (!type.compare("threeStateSensor")) {
		newDevice = new WirelessShutterContact(deviceID, RefreshTime::DISABLED);
	}
	else if (!type.compare("powerMeter")) {
		newDevice = new PowerMeterSwitch(deviceID, RefreshTime::DISABLED);
	}
	else if (!type.compare("thermostat")) {
		newDevice = new RadiatorThermostat(deviceID, RefreshTime::DISABLED);
	}
	else
		throw IllegalStateException("unsupported device " + type);

	m_devices.emplace(deviceID, newDevice);

	const auto description = DeviceDescription::Builder()
		.id(newDevice->id())
		.type(newDevice->vendor(), newDevice->productName())
		.modules(newDevice->moduleTypes())
		.refreshTime(newDevice->refreshTime())
		.build();
	dispatch(new NewDeviceCommand(description));
}

void ConradDeviceManager::stop()
{
	DeviceManager::stop();
	answerQueue().dispose();
}

AsyncWork<>::Ptr ConradDeviceManager::startDiscovery(const Timespan &timeout)
{
	auto work = BlockingAsyncWork<>::instance();

	string request = "set CUL_0 hmPairForSec " + to_string(timeout.totalSeconds());
	m_fhemClient->sendRequest(request);

	return work;
}

void ConradDeviceManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	ScopedLock<FastMutex> lock(m_devicesMutex);

	auto it = m_devices.find(cmd->deviceID());
	if (it == m_devices.end())
		throw NotFoundException("accept: " + cmd->deviceID().toString());

	DeviceManager::handleAccept(cmd);
}

AsyncWork<set<DeviceID>>::Ptr ConradDeviceManager::startUnpair(
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

		// conrad ID is filled in the last 6 characters of Device ID
		string conradID = id.toString().substr(12, id.toString().size());
		// must be formated as upper case to be acceptable by unpair command
		for (auto & c: conradID) c = toupper(c);

		string request = "delete HM_" + conradID;
		m_fhemClient->sendRequest(request);

		auto it = m_devices.find(id);
		if (it != m_devices.end())
			m_devices.erase(id);

		work->setResult({id});
	}

	return work;
}

void ConradDeviceManager::fireMessage(
	DeviceID const &deviceID,
	const JSON::Object::Ptr message)
{
	try {
		ConradEvent event = ConradEvent::parse(deviceID, message);
		m_eventSource.fireEvent(event, &ConradListener::onConradMessage);
	}
	catch (Exception& e) {
		logger().warning("failed to obtain information from message");
		logger().log(e, __FILE__, __LINE__);
	}
}
