#include <zmq.h>

#include "commands/DeviceUnpairCommand.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceAcceptCommand.h"
#include "commands/NewDeviceCommand.h"
#include "commands/GatewayListenCommand.h"
#include "conrad/ConradDeviceManager.h"
#include "conrad/PowerMeterSwitch.h"
#include "conrad/RadiatorThermostat.h"
#include "conrad/WirelessShutterContact.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "loop/StopControl.h"
#include "model/SensorData.h"
#include "model/DevicePrefix.h"

#define CONRAD_VENDOR "Conrad"
#define RSP_BUFFER_SIZE 129070

BEEEON_OBJECT_BEGIN(BeeeOn, ConradDeviceManager)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(DeviceStatusHandler)
BEEEON_OBJECT_PROPERTY("deviceCache", &ConradDeviceManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("distributor", &ConradDeviceManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &ConradDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("cmdZmqIface", &ConradDeviceManager::setCmdZmqIface)
BEEEON_OBJECT_PROPERTY("eventZmqIface", &ConradDeviceManager::setEventZmqIface)
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

void ConradDeviceManager::setCmdZmqIface(const string &cmdZmqIface)
{
	m_cmdZmqIface = URI(cmdZmqIface);
}

void ConradDeviceManager::setEventZmqIface(const string &eventZmqIface)
{
	m_eventZmqIface = URI(eventZmqIface);
}

void ConradDeviceManager::run()
{
	logger().information("starting Conrad device manager", __FILE__, __LINE__);

	char buffer[RSP_BUFFER_SIZE];
	void *context = zmq_ctx_new();
	void *subscriber = zmq_socket(context, ZMQ_SUB);
	zmq_connect(subscriber, m_eventZmqIface.toString().c_str());
	zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, "", 0);

	StopControl::Run run(m_stopControl);
	while (run) {
		int size = zmq_recv(subscriber, buffer, RSP_BUFFER_SIZE, 0);
		if (size < 0)
			continue;
		buffer[size] = '\0';

		try {
			processMessage(buffer);
		}
		catch (const Exception& e) {
			logger().log(e, __FILE__, __LINE__);
		}
	}

	logger().information("stopping Conrad device manager", __FILE__, __LINE__);

	zmq_close(subscriber);
	zmq_ctx_destroy(context);
}

void ConradDeviceManager::processMessage(const string &message)
{
	Object::Ptr object = JsonUtil::parse(message);
	if (!object->has("dev"))
		throw IllegalStateException("message does not contain 'dev' element");

	uint32_t id = NumberParser::parseHex(object->getValue<string>("dev"));
	DeviceID deviceID = DeviceID(DevicePrefix::PREFIX_CONRAD, id);

	if (logger().debug()) {
		logger().debug("Event " + object->getValue<string>("event") +
			" from " + deviceID.toString(), __FILE__, __LINE__);
	}

	ScopedLock<FastMutex> lock(m_devicesMutex);

	if (!object->getValue<string>("event").compare("new_device")) {
		createNewDeviceUnlocked(deviceID, object->getValue<string>("type"));
	}
	else if (!object->getValue<string>("event").compare("message")) {
		auto it = m_devices.find(deviceID);
		if (it == m_devices.end())
			createNewDeviceUnlocked(deviceID, object->getValue<string>("type"));

		SensorData data = it->second->parseMessage(object);

		if (!deviceCache()->paired(deviceID))
			return;

		try {
			if (data.size() > 0)
				ship(data);
		}
		catch (const Exception& e) {
			logger().log(e, __FILE__, __LINE__);
		}
	}
	else {
		throw IllegalStateException("unknown message");
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
	Object::Ptr obj = new Object();
	ostringstream cmdStream;

	obj->set("cmd", "pair");
	obj->set("tout", to_string(timeout.totalSeconds()));

	obj->stringify(cmdStream);
	sendCmdRequest(cmdStream.str());

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
	Object::Ptr obj = new Object();
	ostringstream cmdStream;

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

		obj->set("cmd", "unpair");
		obj->set("device", conradID);

		obj->stringify(cmdStream);
		sendCmdRequest(cmdStream.str());

		auto it = m_devices.find(id);
		if (it != m_devices.end())
			m_devices.erase(id);

		work->setResult({id});
	}

	return work;
}

Object::Ptr ConradDeviceManager::sendCmdRequest(const string &request)
{
	void *context = zmq_ctx_new();
	void *requester = zmq_socket (context, ZMQ_REQ);
	zmq_connect(requester, m_cmdZmqIface.toString().c_str());

	char buffer[RSP_BUFFER_SIZE];
	zmq_send(requester, request.c_str(), request.length(), 0);
	zmq_recv(requester, buffer, RSP_BUFFER_SIZE, 0);

	zmq_close(requester);
	zmq_ctx_destroy(context);

	return JsonUtil::parse(string(buffer));
}
