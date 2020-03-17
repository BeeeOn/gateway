#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>
#include <Poco/ScopedLock.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Timespan.h>
#include <Poco/Timestamp.h>
#include <Poco/JSON/Object.h>

#include "commands/DeviceUnpairCommand.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceAcceptCommand.h"
#include "commands/NewDeviceCommand.h"
#include "commands/GatewayListenCommand.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "loop/StopControl.h"
#include "model/SensorData.h"
#include "model/DevicePrefix.h"
#include "sonoff/SonoffDeviceManager.h"
#include "sonoff/SonoffSC.h"
#include "util/BlockingAsyncWork.h"
#include "util/JsonUtil.h"

#define SONOFF_VENDOR "Sonoff"

BEEEON_OBJECT_BEGIN(BeeeOn, SonoffDeviceManager)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(DeviceStatusHandler)
BEEEON_OBJECT_PROPERTY("deviceCache", &SonoffDeviceManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("distributor", &SonoffDeviceManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &SonoffDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("mqttClient", &SonoffDeviceManager::setMqttClient)
BEEEON_OBJECT_PROPERTY("maxLastSeen", &SonoffDeviceManager::setMaxLastSeen)
BEEEON_OBJECT_END(BeeeOn, SonoffDeviceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::JSON;
using namespace std;

static const Timespan DISCOVER_IDLE = 5 * Timespan::SECONDS;

SonoffDeviceManager::SonoffDeviceManager() :
	DeviceManager(DevicePrefix::PREFIX_SONOFF, {
		typeid(GatewayListenCommand),
		typeid(DeviceAcceptCommand),
		typeid(DeviceUnpairCommand),
		typeid(DeviceSetValueCommand),
	}),
	m_maxLastSeen(10 * Timespan::MINUTES)
{
}

void SonoffDeviceManager::setMqttClient(MqttClient::Ptr mqttClient)
{
	m_mqttClient = mqttClient;
}

void SonoffDeviceManager::setMaxLastSeen(const Timespan &timeout)
{
	if (timeout.totalSeconds() <= 0)
		throw InvalidArgumentException("scan timeout time must be at least a second");

	m_maxLastSeen = timeout;
}

void SonoffDeviceManager::run()
{
	logger().information("starting Sonoff device manager", __FILE__, __LINE__);

	StopControl::Run run(m_stopControl);
	while (run) {
		try {
			MqttMessage mqttMessage = m_mqttClient->receive(-1);
			if (!mqttMessage.message().empty())
				processMQTTMessage(mqttMessage);
		}
		catch (const Exception& e) {
			logger().log(e, __FILE__, __LINE__);
		}
	}

	logger().information("stopping Sonoff device manager", __FILE__, __LINE__);
}

void SonoffDeviceManager::stop()
{
	DeviceManager::stop();
	answerQueue().dispose();
}

void SonoffDeviceManager::processMQTTMessage(const MqttMessage &message)
{
	if (logger().trace()) {
		logger().dump(
			"received message on topic " + message.topic() +
			" of size " + to_string(message.message().size()) + " B",
			message.message().data(),
			message.message().size(),
			Message::PRIO_TRACE);
	}

	auto deviceInfo = retrieveDeviceInfo(message.message());

	auto it = m_devices.find(deviceInfo.first);
	if (it == m_devices.end())
		createNewDevice(deviceInfo);

	it = m_devices.find(deviceInfo.first);
	SensorData data = it->second->parseMessage(message);

	if (!deviceCache()->paired(deviceInfo.first))
		return;

	try {
		if (data.size() > 0)
			ship(data);
	}
	catch (const Exception& e) {
		logger().log(e, __FILE__, __LINE__);
	}
}

pair<DeviceID, string> SonoffDeviceManager::retrieveDeviceInfo(const std::string &message) const
{
	Object::Ptr object = JsonUtil::parse(message);

	if (!object->has("host"))
		throw IllegalStateException("message do not contain 'host' element");

	string hostName = object->getValue<string>("host");
	StringTokenizer tokenizer(hostName, "_");
	if (tokenizer.count() != 2)
		throw IllegalStateException("'host' element do not contain correct value");

	if (tokenizer[0] != "SONOFFSC")
		throw IllegalStateException("unsupported device " + tokenizer[0]);

	uint32_t id = NumberParser::parseHex(tokenizer[1]);

	return {DeviceID(DevicePrefix::PREFIX_SONOFF, id), tokenizer[0]};
}

void SonoffDeviceManager::createNewDevice(const pair<DeviceID, string>& deviceInfo)
{
	SonoffDevice::Ptr newDevice;

	if (deviceInfo.second == "SONOFFSC")
		newDevice = new SonoffSC(deviceInfo.first, RefreshTime::DISABLED);
	else
		throw IllegalStateException("unsupported device " + deviceInfo.second);

	logger().information("found new device " + deviceInfo.first.toString(), __FILE__, __LINE__);

	m_devices.emplace(deviceInfo.first, newDevice);
}

AsyncWork<>::Ptr SonoffDeviceManager::startDiscovery(const Timespan& duration)
{
	auto work = BlockingAsyncWork<>::instance();
	Clock started;

	while (!m_stopControl.shouldStop()) {
		if (duration - DISCOVER_IDLE - started.elapsed() < 0)
			break;

		for (auto& device : m_devices) {
			if (device.second->lastSeen().elapsed() > m_maxLastSeen.totalMicroseconds())
				continue;

			auto types = device.second->moduleTypes();
			const auto description = DeviceDescription::Builder()
				.id(device.second->id())
				.type(device.second->vendor(), device.second->productName())
				.modules(types)
				.refreshTime(device.second->refreshTime())
				.build();

			dispatch(new NewDeviceCommand(description));
		}

		m_stopControl.waitStoppable(DISCOVER_IDLE);
	}

	return work;
}

void SonoffDeviceManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	ScopedLock<FastMutex> lock(m_devicesMutex);

	auto it = m_devices.find(cmd->deviceID());
	if (it == m_devices.end())
		throw NotFoundException("accept: " + cmd->deviceID().toString());

	DeviceManager::handleAccept(cmd);
}

AsyncWork<set<DeviceID>>::Ptr SonoffDeviceManager::startUnpair(
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

		auto it = m_devices.find(id);
		if (it != m_devices.end())
			m_devices.erase(id);

		work->setResult({id});
	}

	return work;
}
