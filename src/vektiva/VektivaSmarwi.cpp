#include <map>

#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>
#include <Poco/ScopedLock.h>
#include <Poco/String.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Timespan.h>

#include "model/SensorData.h"
#include "vektiva/VektivaSmarwi.h"

#define SMARWI_OPEN_CLOSE_MODULE_ID 0
#define SMARWI_OPEN_TO_MODULE_ID 1
#define SMARWI_FIX_MODULE_ID 2
#define SMARWI_RSSI_MODULE_ID 3
#define SMARWI_OPEN_MAX 100.0
#define SMARWI_OPEN_MIN 0.0
#define SMARWI_CLOSE 0.0
#define SMARWI_OPEN 1.0
#define SMARWI_UNFIX 0.0
#define SMARWI_FIX 1.0

using namespace BeeeOn;
using namespace std;
using namespace Poco;

static list<ModuleType> SMARWI_MODULE_TYPES = {
	ModuleType(ModuleType::Type::TYPE_OPEN_CLOSE,
		{ModuleType::Attribute::ATTR_CONTROLLABLE}),
	ModuleType(ModuleType::Type::TYPE_OPEN_RATIO,
		{ModuleType::Attribute::ATTR_CONTROLLABLE}),
	ModuleType(ModuleType::Type::TYPE_ON_OFF,
		{ModuleType::Attribute::ATTR_CONTROLLABLE}),
	ModuleType(ModuleType::Type::TYPE_RSSI),
};
static const Timespan MOVE_TIME_OUT(35 * Timespan::SECONDS);
static const string PRODUCT_NAME("SmarWi");

VektivaSmarwi::VektivaSmarwi(
		const MACAddress& macAddr,
		const string &remoteID):
	m_remoteID(remoteID),
	m_macAddress(macAddr)
{
	m_deviceId = buildDeviceID(macAddr);
}

list<ModuleType> VektivaSmarwi::moduleTypes() const
{
	return SMARWI_MODULE_TYPES;
}

void VektivaSmarwi::publishModifyStateCommand(
		const ModuleID &moduleID,
		double value,
		MqttClient::Ptr mqttClient)
{
	string cmd;

	switch (moduleID.value()) {
	case SMARWI_OPEN_CLOSE_MODULE_ID:
		if (value == SMARWI_OPEN) {
			cmd = "open";
		}
		else if (value == SMARWI_CLOSE) {
			cmd = "close";
		}
		else {
			throw InvalidArgumentException(
				"unknown value when attempting to open/close window");
		}
		break;
	case SMARWI_OPEN_TO_MODULE_ID:
		if (value > SMARWI_OPEN_MIN && value <= SMARWI_OPEN_MAX) {
			cmd = "open;" + to_string(value);
		}
		else if (value == SMARWI_OPEN_MIN) {
			cmd = "close";
		}
		else {
			throw InvalidArgumentException(
				"unknown value when attempting to open/close window");
		}
		break;
	case SMARWI_FIX_MODULE_ID:
		if (value == SMARWI_FIX) {
			cmd = "fix";
		}
		else if (value == SMARWI_UNFIX) {
			cmd = "stop";
		}
		else {
			throw InvalidArgumentException(
				"unknown value in operation of un/fixing the window");
		}
		break;
	default:
		throw IOException("invalid module ID: " + to_string(moduleID.value()));
	}

	auto macAddr = toLower(m_macAddress.toString());
	mqttClient->publish(buildMqttMessage(m_remoteID, macAddr, cmd));
}

void VektivaSmarwi::confirmStateModification(MqttClient::Ptr mqttClient)
{
	/* Timeout for receiving the message. In case another unrelated message
	 * would be received, it's thrown away and
	 * message has m_receiveTimeout seconds to receive the correct message.
	 * 35 seconds just in case. After 30 seconds,
	 * SMARWI throws movement timeout error.
	 */
	Timestamp startTime;
	Timestamp now;

	while (Timespan(now - startTime) < MOVE_TIME_OUT) {
		MqttMessage rcvmsg;
		try {
			rcvmsg = mqttClient->receive(1 * Timespan::SECONDS);
		}
		catch (TimeoutException &e) {
			now.update();
			continue;
		}

		auto topicRegexString = buildTopicRegex(
			m_remoteID,
			toLower(m_macAddress.toString()),
			"status");
		const RegularExpression topicRegex(topicRegexString);
		if (topicRegex.match(rcvmsg.topic())) {
			string message = rcvmsg.message();
			try {
				auto res = parseStatusResponse(message);
				if (res.error() != 0 || res.ok() != 1 || res.ro() != 0) {
					throw IOException(
						"error occured while attempting to change state of "
						"Vektiva device with device ID: " +
						deviceID().toString());
				}
				else if (res.status() == 250) {
					break;
				}
			}
			catch (const SyntaxException &e) {
				if (logger().debug())
					logger().debug("unable to parse incoming message");
			}
		}
		now.update();
	}

	if (Timespan(now - startTime) >= MOVE_TIME_OUT)
		throw TimeoutException("smarwi status change timed out");
}

void VektivaSmarwi::requestModifyState(
		const ModuleID &moduleID,
		const double value,
		MqttClient::Ptr mqttClient)
{
	publishModifyStateCommand(moduleID, value, mqttClient);
	confirmStateModification(mqttClient);
}

VektivaSmarwiStatus VektivaSmarwi::parseStatusResponse(string &message)
{
	StringTokenizer tokens(message, "\n",
		StringTokenizer::TOK_TRIM | StringTokenizer::TOK_IGNORE_EMPTY);
	map<string, string> tokenMap;
	for (const auto &token : tokens) {
		StringTokenizer keyValue(token, ":",
			StringTokenizer::TOK_TRIM | StringTokenizer::TOK_IGNORE_EMPTY);
		if (keyValue.count() == 2)
			tokenMap.insert(pair<string, string>(keyValue[0], keyValue[1]));
	}

	auto statusValue = NumberParser::parse(tokenMap["s"]);
	auto errorValue = NumberParser::parse(tokenMap["e"]);
	auto okValue = NumberParser::parse(tokenMap["ok"]);
	auto ridgeOutValue = NumberParser::parse(tokenMap["ro"]);
	auto ipValue = (uint32_t) NumberParser::parse(tokenMap["ip"]);
	struct in_addr addr = {ipValue};
	Net::IPAddress ipAddress(inet_ntoa(addr));
	auto fixValue = NumberParser::parse(tokenMap["fix"]);
	auto rssiValue = NumberParser::parse(tokenMap["rssi"]);
	bool positionValue = tokenMap["pos"] == "o";

	VektivaSmarwiStatus smarwiStatus(
		statusValue,
		errorValue,
		okValue,
		ridgeOutValue,
		positionValue,
		fixValue,
		ipAddress,
		rssiValue);

	return smarwiStatus;
}

SensorData VektivaSmarwi::createSensorData(
	const VektivaSmarwiStatus &smarwiStatus)
{
	double isOpen = smarwiStatus.pos() ? 1.0 : 0.0;
	SensorData data;
	data.setDeviceID(m_deviceId);
	data.insertValue({SMARWI_OPEN_CLOSE_MODULE_ID, isOpen});
	data.insertValue({SMARWI_FIX_MODULE_ID, double(smarwiStatus.fix())});
	data.insertValue({SMARWI_RSSI_MODULE_ID, double(smarwiStatus.rssi())});

	return data;
}

DeviceID VektivaSmarwi::buildDeviceID(const MACAddress &macAddr)
{
	return DeviceID(DevicePrefix::PREFIX_VEKTIVA, macAddr);
}

string VektivaSmarwi::remoteID()
{
	return m_remoteID;
}

MACAddress VektivaSmarwi::macAddress()
{
	return m_macAddress;
}

DeviceID VektivaSmarwi::deviceID()
{
	return m_deviceId;
}

MqttMessage VektivaSmarwi::buildMqttMessage(
		const string &remoteId,
		const string &macAddress,
		const string &command)
{
	return MqttMessage("ion/" + remoteId + "/%" + macAddress + "/cmd", command);
}

string VektivaSmarwi::buildTopicRegex(
		const string &remoteId,
		const string &macAddress,
		const string &lastSegment)
{
	return "ion\\/" + remoteId + "\\/%" + macAddress + "\\/" + lastSegment;
}

string VektivaSmarwi::productName()
{
	return  PRODUCT_NAME;
}

Net::IPAddress VektivaSmarwi::ipAddress()
{
	return m_ipAddress;
}

void VektivaSmarwi::setIpAddress(const Net::IPAddress &ipAddress)
{
	m_ipAddress = ipAddress;
}
