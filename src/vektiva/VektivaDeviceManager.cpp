#include <Poco/RegularExpression.h>
#include <Poco/ScopedLock.h>
#include <Poco/Timespan.h>
#include <Poco/Timestamp.h>

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
#include "vektiva/VektivaDeviceManager.h"

#define VEKTIVA_VENDOR "Vektiva"

BEEEON_OBJECT_BEGIN(BeeeOn, VektivaDeviceManager)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(DeviceStatusHandler)
BEEEON_OBJECT_PROPERTY("deviceCache", &VektivaDeviceManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("distributor", &VektivaDeviceManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &VektivaDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("mqttClient", &VektivaDeviceManager::setMqttClient)
BEEEON_OBJECT_PROPERTY("mqttStatusClient", &VektivaDeviceManager::setStatusMqttClient)
BEEEON_OBJECT_PROPERTY("receiveTimeout", &VektivaDeviceManager::setReceiveTimeout)
BEEEON_OBJECT_END(BeeeOn, VektivaDeviceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

VektivaDeviceManager::VektivaDeviceManager() :
	DeviceManager(DevicePrefix::PREFIX_VEKTIVA, {
		typeid(GatewayListenCommand),
		typeid(DeviceAcceptCommand),
		typeid(DeviceUnpairCommand),
		typeid(DeviceSetValueCommand),
	})
{
}

void VektivaDeviceManager::run()
{
	StopControl::Run run(m_stopControl);
	while (run) {
		try {
			MqttMessage rcvmsg = m_mqttStatusClient->receive(-1);
			if (!rcvmsg.message().empty())
				analyzeMessage(rcvmsg);
		}
		catch (const Exception& e) {
			logger().log(e, __FILE__, __LINE__);
		}
	}

	logger().information("stopping Vektiva device manager", __FILE__, __LINE__);
}

void VektivaDeviceManager::stop()
{
	DeviceManager::stop();
	answerQueue().dispose();
}

void VektivaDeviceManager::setReceiveTimeout(const Timespan &timeout)
{
	if (timeout < 1 * Timespan::MILLISECONDS)
		throw InvalidArgumentException("receiveTimeout must be at least 1 ms");

	m_receiveTimeout = timeout;
}

void VektivaDeviceManager::setStatusMqttClient(MqttClient::Ptr mqttClient)
{
	m_mqttStatusClient = mqttClient;
}

void VektivaDeviceManager::setMqttClient(MqttClient::Ptr mqttClient)
{
	m_mqttClient = mqttClient;
}

AsyncWork<>::Ptr VektivaDeviceManager::startDiscovery(const Timespan &timeout)
{
	VektivaSeeker::Ptr seeker = new VektivaSeeker(*this, timeout);
	seeker->start();
	return seeker;
}

VektivaDeviceManager::VektivaSeeker::VektivaSeeker(
	VektivaDeviceManager &parent,
	const Timespan &duration) :
	AbstractSeeker(duration),
	m_parent(parent)
{
}

void VektivaDeviceManager::VektivaSeeker::seekLoop(StopControl &)
{
	for (auto device : m_parent.m_devices) {
		if (m_parent.updateDevice(device.second))
			m_parent.dispatchNewDevice(device.second);
	}
}

bool VektivaDeviceManager::updateDevice(VektivaSmarwi::Ptr newDevice)
{
	ScopedLockWithUnlock<FastMutex> clientMqtt(m_clientMqttMutex);
	m_mqttClient->publish(
		VektivaSmarwi::buildMqttMessage(
			newDevice->remoteID(),
			toLower(newDevice->macAddress().toString()),
			"status")
	);
	clientMqtt.unlock();

	// if no status message is received, no actions are done further
	if (!receiveStatusMessageAndUpdate(newDevice))
		return false;

	FastMutex::ScopedLock lock(m_devicesMutex);
	auto result = m_devices.emplace(newDevice->deviceID(), newDevice);
	if (!result.second) {
		result.first->second = newDevice;
		logger().information(
			"device " + newDevice->deviceID().toString() + " updated",
			__FILE__, __LINE__);
	}
	else {
		logger().information(
			"new device " + newDevice->deviceID().toString() + " found",
			__FILE__, __LINE__);
	}

	return true;
}

void VektivaDeviceManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	FastMutex::ScopedLock lock(m_devicesMutex);

	auto it = m_devices.find(cmd->deviceID());
	if (it == m_devices.end()) {
		throw NotFoundException(
			"device " + cmd->deviceID().toString() + " could not be accepted");
	}
	else {
		logger().information(
			"device " + cmd->deviceID().toString() + " accepted",
			__FILE__, __LINE__);
	}

	DeviceManager::handleAccept(cmd);
}

AsyncWork<set<DeviceID>>::Ptr VektivaDeviceManager::startUnpair(
		const DeviceID &id,
		const Timespan &)
{
	FastMutex::ScopedLock lock(m_devicesMutex);

	if (!deviceCache()->paired(id)) {
		logger().warning(
			"unpairing device " + id.toString() + " that is not paired",
			__FILE__, __LINE__);

		return BlockingAsyncWork<set<DeviceID>>::instance();
	}
	else {
		deviceCache()->markUnpaired(id);

		auto itDevice = m_devices.find(id);
		if (itDevice != m_devices.end())
			m_devices.erase(id);
	}

	logger().information("successfully unpaired device " + id.toString(),
		__FILE__,__LINE__);

	auto work = BlockingAsyncWork<set<DeviceID>>::instance();
	work->setResult({id});

	return work;
}

pair<string, string> VektivaDeviceManager::retrieveDeviceInfoFromTopic(
		const string &topic)
{
	RegularExpression remoteIdRegex(R"(^ion\/([^#\/]+)\/%)");
	RegularExpression macAddrRegex("\\/%([a-fA-F0-9]{12})\\/");

	RegularExpression::MatchVec remoteIdPosVec;
	remoteIdRegex.match(topic, 0, remoteIdPosVec);

	RegularExpression::MatchVec macAddrPosVec;
	macAddrRegex.match(topic, 0, macAddrPosVec);

	auto remoteID = topic.substr(
		remoteIdPosVec[1].offset,
		remoteIdPosVec[1].length);
	auto macAddrString = topic.substr(
		macAddrPosVec[1].offset,
		macAddrPosVec[1].length);
	return pair<string, string>(remoteID, macAddrString);
}

bool VektivaDeviceManager::isTopicValid(
		const string &topic,
		const string &lastSegment,
		const string &remoteId,
		const string &macAddr)
{
	string remoteIdRegex;
	if (remoteId.length() > 0) {
		remoteIdRegex = remoteId;
		escapeRegexString(remoteIdRegex);
	}
	else {
		remoteIdRegex = "[^#+/]+";
	}

	string macAddrRegex;
	if (macAddr .length() > 0) {
		macAddrRegex = macAddr;
		escapeRegexString(macAddrRegex );
	}
	else {
		macAddrRegex = "[a-fA-F0-9]{12}";
	}

	const RegularExpression topicRegex("^ion\\/"
		+ remoteIdRegex + "\\/%" + macAddrRegex + "\\/" + lastSegment + "$");
	return topicRegex.match(topic);
}

void VektivaDeviceManager::escapeRegexString(string &regexString) {
	RegularExpression charsToEscape("([-[\\]{}()*+?.,\\^$|#\\s])");
	charsToEscape.subst(regexString, "\\$1", RegularExpression::RE_GLOBAL);
}


AsyncWork<double>::Ptr VektivaDeviceManager::startSetValue(
		const DeviceID &id,
		const ModuleID &module,
		const double value,
		const Timespan &)
{
	clearMqttMessageBuffer();
	modifyValue(id, module, value);

	logger().information("success to change state of device " + id.toString(),
		__FILE__, __LINE__);

	auto work = BlockingAsyncWork<double>::instance();
	work->setResult(value);

	return work;
}

void VektivaDeviceManager::modifyValue(
		const DeviceID &deviceID,
		const ModuleID &moduleID,
		const double value)
{
	FastMutex::ScopedLock lock(m_devicesMutex);

	auto it = m_devices.find(deviceID);
	if (it == m_devices.end()) {
		throw InvalidArgumentException("no such device: " + deviceID.toString());
	} else {
		ScopedLock<FastMutex> modifyStateMutex(m_clientMqttMutex);
		it->second->requestModifyState(moduleID, value, m_mqttClient);
	}
}

void VektivaDeviceManager::analyzeMessage(MqttMessage &mqttMessage)
{
	auto topic = mqttMessage.topic();
	if (isTopicValid(topic, "status"))
		statusMessageAction(mqttMessage);
	else if (isTopicValid(topic, "online"))
		onlineMessageAction(mqttMessage);
}

void VektivaDeviceManager::statusMessageAction(MqttMessage &mqttMessage)
{
	pair<string, string> deviceInfo = retrieveDeviceInfoFromTopic(
		mqttMessage.topic());
	auto macAddr = MACAddress(NumberParser::parseHex64(deviceInfo.second));
	auto deviceId = VektivaSmarwi::buildDeviceID(macAddr);
	if (!deviceCache()->paired(deviceId))
		return;

	auto message = mqttMessage.message();
	try {
		auto smarwiStatus = VektivaSmarwi::parseStatusResponse(message);
		if (smarwiStatus.status() != 250)
			return;
		shipSmarwiStatus(deviceId, smarwiStatus);
	}
	catch (SyntaxException &e) {
		if (logger().debug())
			logger().debug("unable to parse incoming message");
	}
}

void VektivaDeviceManager::onlineMessageAction(MqttMessage &mqttMessage)
{
	if (mqttMessage.message() != "1")
		return;

	pair<string, string> deviceInfo = retrieveDeviceInfoFromTopic(
		mqttMessage.topic());
	auto remoteId = deviceInfo.first;
	auto macAddrString = deviceInfo.second;

	MACAddress macAddr = MACAddress(NumberParser::parseHex64(macAddrString));
	VektivaSmarwi::Ptr devicePtr = new VektivaSmarwi(macAddr, remoteId);
	updateDevice(devicePtr);
}

MqttMessage VektivaDeviceManager::messageReceivedInTime(
		const string &lastSegment,
		VektivaSmarwi::Ptr device)
{
	clearMqttMessageBuffer();
	/*
	 * Timeout for receiving the message. In case another unrelated message
	 * would be received, it's thrown away and the Gateway has m_receiveTimeout
	 * seconds to receive the correct message.
	 */
	Timestamp startTime;
	Timestamp now;
	MqttMessage rcvmsg;

	while (Timespan(now - startTime) < m_receiveTimeout) {
		try {
			ScopedLock<FastMutex> rcvMsgMutex(m_clientMqttMutex);
			rcvmsg = m_mqttClient->receive(m_receiveTimeout);
		}
		catch (const TimeoutException &e) {
			now.update();
			continue;
		}

		auto topic = rcvmsg.topic();
		auto message = rcvmsg.message();

		if (logger().trace()) {
			logger().dump(
				"received message on topic " + topic +
				" of size " + to_string(message.size()) + " B",
				message.data(),
				message.size(),
				Message::PRIO_TRACE);
		}

		if (isTopicValid(topic, lastSegment, device->remoteID(),
			toLower(device->macAddress().toString()))) {
			return rcvmsg;
		}

		now.update();
	}

	throw TimeoutException("status receive for the device "
		+ device->deviceID().toString() + " timed out");
}

void VektivaDeviceManager::clearMqttMessageBuffer()
{
	ScopedLock<FastMutex> mqttMutex(m_clientMqttMutex);

	MqttMessage rcvmsg("1", "1", MqttMessage::EXACTLY_ONCE);//random init data
	while (rcvmsg.message() != "") {
		try {
			rcvmsg = m_mqttClient->receive(0);
		}
		catch (TimeoutException &e) {
			break;
		}
	}
}

void VektivaDeviceManager::updateDeviceInfo(
		VektivaSmarwi::Ptr device,
		const VektivaSmarwiStatus &smarwiStatus)
{
	device->setIpAddress(smarwiStatus.ipAddress());
}

bool VektivaDeviceManager::receiveStatusMessageAndUpdate(
		VektivaSmarwi::Ptr device)
{
	MqttMessage mqttMessage;
	try {
		mqttMessage = messageReceivedInTime("status", device);
	}
	catch (TimeoutException &e) {
		logger().log(e, __FILE__, __LINE__);
		return false;
	}

	auto message = mqttMessage.message();
	auto smarwiStatus = VektivaSmarwi::parseStatusResponse(message);
	updateDeviceInfo(device, smarwiStatus);

	return true;
}

void VektivaDeviceManager::shipSmarwiStatus(
		const DeviceID &deviceId,
		const VektivaSmarwiStatus &smarwiStatus)
{
	auto it = m_devices.find(deviceId);
	if (it == m_devices.end()) {
		if (logger().debug()) {
			logger().debug(
				"ship data of device " +
				deviceId.toString() + " paired but not instantiated",
				__FILE__, __LINE__);
		}

		return;
	}
	try {
		SensorData data = it->second->createSensorData(smarwiStatus);
		ship(data);
	}
	BEEEON_CATCH_CHAIN(logger());
}

void VektivaDeviceManager::dispatchNewDevice(VektivaSmarwi::Ptr device)
{
	auto builder = DeviceDescription::Builder();
	const auto description = builder
		.id(device->deviceID())
		.macAddress(device->macAddress())
		.ipAddress(device->ipAddress())
		.type(VEKTIVA_VENDOR, device->productName())
		.modules(device->moduleTypes())
		.build();

	dispatch(new NewDeviceCommand(description));
}
