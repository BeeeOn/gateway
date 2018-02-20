#include <arpa/inet.h>
#include <netinet/in.h>

#include <Poco/DigestStream.h>
#include <Poco/Event.h>
#include <Poco/NumberFormatter.h>
#include <Poco/Timestamp.h>
#include <Poco/Net/IPAddress.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/RegularExpression.h>
#include <Poco/SHA1Engine.h>
#include <Poco/StreamCopier.h>
#include <Poco/String.h>

#include "model/DevicePrefix.h"
#include "net/HTTPUtil.h"
#include "util/JsonUtil.h"
#include "vpt/VPTBoilerModuleType.h"
#include "vpt/VPTDevice.h"
#include "vpt/VPTValuesParser.h"
#include "vpt/VPTZoneModuleType.h"

#define VPT_VENDOR "Thermona"
#define EXTRACT_ZONE_MASK 0x0007000000000000
#define OMIT_SUBDEVICE_MASK 0xff00ffffffffffff
#define MAX_ATTEMPTS 3
#define SETTING_DELAY_MS 15000
#define GATEWAY_ID_MASK 0x00ffffffffffffffUL

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::JSON;
using namespace Poco::Net;
using namespace std;

/*
 * Registers belong gradually to zones 1, 2, 3, 4.
 */
const vector<string> VPTDevice::REG_BOILER_OPER_TYPE =
	{"PE040","PE041","PE042","PE043"};

const vector<string> VPTDevice::REG_BOILER_OPER_MODE =
	{"PE044","PE045","PE046","PE047"};

const vector<string> VPTDevice::REG_MAN_ROOM_TEMP =
	{"PE086","PE087","PE088","PE089"};

const vector<string> VPTDevice::REG_MAN_WATER_TEMP =
	{"PE094","PE095","PE096","PE097"};

const vector<string> VPTDevice::REG_MAN_TUV_TEMP =
	{"PE098","PE099","PE100","PE101"};

const vector<string> VPTDevice::REG_MOD_WATER_TEMP =
	{"PE075","PE076","PE077","PE078"};

const std::list<ModuleType> VPTDevice::ZONE_MODULE_TYPES = {
	// MOD_BOILER_OPERATION_TYPE
	ModuleType(ModuleType::Type::TYPE_ENUM,
		VPTZoneModuleType(VPTZoneModuleType::MOD_BOILER_OPERATION_TYPE).toString(),
		{ModuleType::Attribute::ATTR_CONTROLLABLE}),
	// MOD_BOILER_OPERATION_MODE
	ModuleType(ModuleType::Type::TYPE_ENUM,
		VPTZoneModuleType(VPTZoneModuleType::MOD_BOILER_OPERATION_MODE).toString(),
		{ModuleType::Attribute::ATTR_CONTROLLABLE}),
	// MOD_REQUESTED_ROOM_TEMPERATURE
	ModuleType(ModuleType::Type::TYPE_TEMPERATURE, {ModuleType::Attribute::ATTR_INNER}),
	// MOD_CURRENT_ROOM_TEMPERATURE
	ModuleType(ModuleType::Type::TYPE_TEMPERATURE, {ModuleType::Attribute::ATTR_INNER}),
	// MOD_REQUESTED_WATER_TEMPERATURE_SET
	ModuleType(ModuleType::Type::TYPE_TEMPERATURE, {ModuleType::Attribute::ATTR_CONTROLLABLE}),
	// MOD_CURRENT_WATER_TEMPERATURE
	ModuleType(ModuleType::Type::TYPE_TEMPERATURE),
	// MANUAL_REQUESTED_ROOM_TEMPERATURE
	ModuleType(ModuleType::Type::TYPE_TEMPERATURE,
		{ModuleType::Attribute::ATTR_INNER, ModuleType::Attribute::ATTR_CONTROLLABLE}),
	// MANUAL_REQUESTED_WATER_TEMPERATURE
	ModuleType(ModuleType::Type::TYPE_TEMPERATURE, {ModuleType::Attribute::ATTR_CONTROLLABLE}),
	// MANUAL_REQUESTED_TUV_TEMPERATURE
	ModuleType(ModuleType::Type::TYPE_TEMPERATURE, {ModuleType::Attribute::ATTR_CONTROLLABLE})
};

const std::list<ModuleType> VPTDevice::BOILER_MODULE_TYPES = {
	// MOD_BOILER_STATUS
	ModuleType(ModuleType::Type::TYPE_ENUM,
		VPTBoilerModuleType(VPTBoilerModuleType::MOD_BOILER_STATUS).toString()),
	// MOD_BOILER_MODE
	ModuleType(ModuleType::Type::TYPE_ENUM,
		VPTBoilerModuleType(VPTBoilerModuleType::MOD_BOILER_MODE).toString()),
	// MOD_CURRENT_WATER_TEMPERATURE
	ModuleType(ModuleType::Type::TYPE_TEMPERATURE),
	// MOD_CURRENT_OUTSIDE_TEMPERATURE
	ModuleType(ModuleType::Type::TYPE_TEMPERATURE, {ModuleType::Attribute::ATTR_OUTER}),
	// MOD_AVERAGE_OUTSIDE_TEMPERATURE
	ModuleType(ModuleType::Type::TYPE_TEMPERATURE, {ModuleType::Attribute::ATTR_OUTER}),
	// MOD_CURRENT_BOILER_PERFORMANCE
	ModuleType(ModuleType::Type::TYPE_PERFORMANCE),
	// MOD_CURRENT_BOILER_PRESSURE
	ModuleType(ModuleType::Type::TYPE_PRESSURE),
	// MOD_CURRENT_BOILER_OT_FAULT_FLAGS
	ModuleType(ModuleType::Type::TYPE_BITMAP,
		VPTBoilerModuleType(VPTBoilerModuleType::MOD_CURRENT_BOILER_OT_FAULT_FLAGS).toString()),
	// MOD_CURRENT_BOILER_OT_OEM_FAULTS
	ModuleType(ModuleType::Type::TYPE_BITMAP,
		VPTBoilerModuleType(VPTBoilerModuleType::MOD_CURRENT_BOILER_OT_OEM_FAULTS).toString()),
};

const int VPTDevice::COUNT_OF_ZONES = 4;


VPTDevice::VPTDevice(const SocketAddress& address):
	m_address(address)
{
}

VPTDevice::VPTDevice()
{
}

DeviceID VPTDevice::deviceID() const
{
	return m_deviceId;
}

SocketAddress VPTDevice::address() const
{
	return m_address;
}

void VPTDevice::setAddress(const SocketAddress& address)
{
	m_address = address;
}

void VPTDevice::setPassword(const string& pwd)
{
	m_password = pwd;
}

FastMutex& VPTDevice::lock()
{
	return m_lock;
}

string VPTDevice::generateStamp(const Action action)
{
	string stamp;
	StreamSocket ping;

	struct in_addr *addr = NULL;
	uint32_t ipAddress = 0;

	try {
		ping.connect(m_address, m_pingTimeout);

		addr = (struct in_addr *) ping.address().host().addr();
		ipAddress = (uint32_t) addr->s_addr;
	}
	catch (const Exception& e) {
		logger().warning("unable to get IP address of proper gateway's interface, IP 0.0.0.0 is used",
			__FILE__, __LINE__);
	}
	ping.close();

	const uint64_t id = m_gatewayID.data() & GATEWAY_ID_MASK;

	const uint32_t time = (uint32_t) Timestamp().epochTime();

	//Format: 4B - GATEWAY IP, 7B - GATEWAY ID, 4B - TIME, 1B - ACTION
	//Hex: 8 + 14 + 8 + 2 = 32 characters
	NumberFormatter::appendHex(stamp, ipAddress, 8);
	NumberFormatter::appendHex(stamp, id, 14);
	NumberFormatter::appendHex(stamp, time, 8);
	NumberFormatter::appendHex(stamp, action, 2);

	return stamp;
}

void VPTDevice::stampVPT(const Action action)
{
	string stamp = generateStamp(action);

	try {
		prepareAndSendRequest("BEEE0", stamp);

		logger().debug("update register BEEE0 to " + stamp, __FILE__, __LINE__);
	}
	catch (const Exception& e) {
		logger().warning("the BEEE0 register update failed due to network", __FILE__, __LINE__);
	}
}

bool VPTDevice::operator==(const VPTDevice& other) const
{
	return other.deviceID() == m_deviceId;
}

VPTDevice::Ptr VPTDevice::buildDevice(const SocketAddress& address,
	const Timespan& httpTimeout, const Timespan& pingTimeout, const GatewayID& id)
{
	VPTDevice::Ptr device = new VPTDevice(address);
	device->m_httpTimeout = httpTimeout;
	device->m_pingTimeout = pingTimeout;
	device->m_gatewayID = id;
	device->buildDeviceID();
	return device;
}

void VPTDevice::buildDeviceID()
{
	HTTPRequest request;
	request.setMethod(HTTPRequest::HTTP_GET);
	request.setURI("/values.json");
	HTTPEntireResponse response = sendRequest(request, m_httpTimeout);

	Object::Ptr object = JsonUtil::parse(response.getBody());
	string mac = object->getValue<string>("id");
	m_deviceId = DeviceID(DevicePrefix::PREFIX_VPT, NumberParser::parseHex64(mac));

	stampVPT(Action::PAIR);
}

void VPTDevice::requestModifyState(const DeviceID& id, const ModuleID module,
	const double value)
{
	const int zone = VPTDevice::extractSubdeviceFromDeviceID(id);
	if (zone == 0) {
		throw InvalidArgumentException(
			"attempt to modify state of invalid zone 0");
	}

	switch (module.value()) {
	case VPTZoneModuleType::MOD_BOILER_OPERATION_TYPE:
		requestSetModBoilerOperationType(zone, value);
		break;
	case VPTZoneModuleType::MOD_BOILER_OPERATION_MODE:
		requestSetModBoilerOperationMode(zone, value);
		break;
	case VPTZoneModuleType::MANUAL_REQUESTED_ROOM_TEMPERATURE:
		requestSetManualRoomTemperature(zone, value);
		break;
	case VPTZoneModuleType::MANUAL_REQUESTED_WATER_TEMPERATURE:
		requestSetManualWaterTemperature(zone, value);
		break;
	case VPTZoneModuleType::MANUAL_REQUESTED_TUV_TEMPERATURE:
		requestSetManualTUVTemperature(zone, value);
		break;
	case VPTZoneModuleType::MOD_REQUESTED_WATER_TEMPERATURE_SET:
		requestSetModWaterTemperature(zone, value);
		break;
	default:
		throw InvalidArgumentException("attempt to set module "
				+ module.toString() + " that is not controllable");
	}
}

void VPTDevice::requestSetModBoilerOperationType(const int zone, const double value)
{
	string registr = REG_BOILER_OPER_TYPE.at(zone - 1);

	string strValue;
	for (auto item : VPTValuesParser::BOILER_OPERATION_TYPE) {
		if (item.second == static_cast<int>(value))
			strValue = item.first;
	}

	if (strValue.empty()) {
		throw InvalidArgumentException(
			"value " + to_string(value)
			+ " is invalid for BOILER_OPERATION_TYPE");
	}

	prepareAndSendRequest(registr, strValue);

	stampVPT(Action::SET);

	/*
	 * The success of request is probed in iterations with
	 * some delay because the setting time of request can be long.
	 */
	for (int i = 0; i < MAX_ATTEMPTS; i++) {
		Event event;
		event.tryWait(SETTING_DELAY_MS / MAX_ATTEMPTS);

		HTTPRequest request;
		request.setMethod(HTTPRequest::HTTP_GET);
		request.setURI("/values.json");
		HTTPEntireResponse response = sendSetRequest(request);

		string newValue = parseZoneAttrFromJson(response.getBody(), zone,
				VPTZoneModuleType(VPTZoneModuleType::MOD_BOILER_OPERATION_TYPE).toString());

		if (strValue == newValue)
			return;
	}

	throw TimeoutException("tried " + to_string(MAX_ATTEMPTS)
			+ " to set BOILER_OPERATION_TYPE");
}

void VPTDevice::requestSetModBoilerOperationMode(const int zone, const double value)
{
	string registr = REG_BOILER_OPER_MODE.at(zone - 1);

	string strValue;
	for (auto item : VPTValuesParser::BOILER_OPERATION_MODE) {
		if (item.second == static_cast<int>(value))
			strValue = item.first;
	}

	if (strValue.empty()) {
		throw InvalidArgumentException(
			"value " + to_string(value)
			+ " is invalid for BOILER_OPERATION_MODE");
	}

	HTTPEntireResponse response = prepareAndSendRequest(registr, strValue);

	string newValue = parseZoneAttrFromJson(response.getBody(), zone,
			VPTZoneModuleType(VPTZoneModuleType::MOD_BOILER_OPERATION_MODE).toString());

	stampVPT(Action::SET);

	if (strValue != newValue) {
		throw IllegalStateException(
			"failed to set BOILER_OPERATION_MODE to " + to_string(value));
	}
}

void VPTDevice::requestSetManualRoomTemperature(const int zone, const double value)
{
	string registr = REG_MAN_ROOM_TEMP.at(zone - 1);
	string strValue = NumberFormatter::format(value, 1);
	strValue = replace(strValue, ".", "%2C");

	HTTPEntireResponse response = prepareAndSendRequest(registr, strValue);

	string newValue = parseZoneAttrFromJson(response.getBody(), zone,
			VPTZoneModuleType(VPTZoneModuleType::MANUAL_REQUESTED_ROOM_TEMPERATURE).toString());

	stampVPT(Action::SET);

	if (value != NumberParser::parseFloat(newValue, ',', '.')) {
		throw IllegalStateException(
			"failed to set MANUAL_REQUESTED_ROOM_TEMPERATURE to " + to_string(value));
	}
}

void VPTDevice::requestSetManualWaterTemperature(const int zone, const double value)
{
	string registr = REG_MAN_WATER_TEMP.at(zone - 1);
	string strValue = NumberFormatter::format(value, 0);

	HTTPEntireResponse response = prepareAndSendRequest(registr, strValue);

	string newValue = parseZoneAttrFromJson(response.getBody(), zone,
			VPTZoneModuleType(VPTZoneModuleType::MANUAL_REQUESTED_WATER_TEMPERATURE).toString());

	stampVPT(Action::SET);

	if (value != NumberParser::parseFloat(newValue, ',', '.')) {
		throw IllegalStateException(
			"failed to set MANUAL_REQUESTED_WATER_TEMPERATURE to " + to_string(value));
	}
}

void VPTDevice::requestSetManualTUVTemperature(const int zone, const double value)
{
	string registr = REG_MAN_TUV_TEMP.at(zone - 1);
	string strValue = NumberFormatter::format(value, 0);

	HTTPEntireResponse response = prepareAndSendRequest(registr, strValue);

	string newValue = parseZoneAttrFromJson(response.getBody(), zone,
			VPTZoneModuleType(VPTZoneModuleType::MANUAL_REQUESTED_TUV_TEMPERATURE).toString());

	stampVPT(Action::SET);

	if (value != NumberParser::parseFloat(newValue, ',', '.')) {
		throw IllegalStateException(
			"failed to set MANUAL_REQUESTED_TUV_TEMPERATURE to " + to_string(value));
	}
}

void VPTDevice::requestSetModWaterTemperature(const int zone, const double value)
{
	string registr = REG_MOD_WATER_TEMP.at(zone - 1);
	string strValue = NumberFormatter::format(value, 0);

	HTTPEntireResponse response = prepareAndSendRequest(registr, strValue);

	string newValue = parseZoneAttrFromJson(response.getBody(), zone,
			VPTZoneModuleType(VPTZoneModuleType::MOD_REQUESTED_WATER_TEMPERATURE_SET).toString());

	stampVPT(Action::SET);

	if (value != NumberParser::parseFloat(newValue, ',', '.')) {
		throw IllegalStateException(
			"failed to set MANUAL_REQUESTED_WATER_TEMPERATURE_SET to " + to_string(value));
	}
}

HTTPEntireResponse VPTDevice::prepareAndSendRequest(const string& registr, const string& value)
{
	HTTPRequest request;
	request.setMethod(HTTPRequest::HTTP_GET);
	request.setURI("/values.json?" + registr + "=" + value);
	return sendSetRequest(request);
}

HTTPEntireResponse VPTDevice::sendSetRequest(HTTPRequest& request)
{
	HTTPEntireResponse response = sendRequest(request, m_httpTimeout);

	try {
		JsonUtil::parse(response.getBody());
	}
	catch (SyntaxException& e) {
		const string nonce = VPTDevice::extractNonce(response.getBody());
		if (nonce.empty())
			throw NotFoundException("nonce was not found in response");

		request.setURI(request.getURI() + "&__HOSTPWD=" +
			VPTDevice::generateHashPassword(m_password, nonce));

		try {
			response = sendRequest(request, m_httpTimeout);
		}
		catch (SyntaxException& e) {
			throw InvalidAccessException("denied access due to bad password", e);
		}
	}

	return response;
}

string VPTDevice::parseZoneAttrFromJson(const string& json, const int zone, const string& key)
{
	try {
		Object::Ptr object = JsonUtil::parse(json);
		object = object->getObject("sensors");
		Object::Ptr sensor = object->getObject("ZONE_" + to_string(zone));
		return sensor->getValue<string>(key);
	}
	catch (SyntaxException& e) {
		logger().log(e, __FILE__, __LINE__);
		logger().warning("can not retrieve data " + key, __FILE__, __LINE__);
		return "";
	}
}

DeviceID VPTDevice::createSubdeviceID(const int zone, const DeviceID& id)
{
	return id | ((uint64_t)zone << 48);
}

DeviceID VPTDevice::omitSubdeviceFromDeviceID(const DeviceID& id)
{
	VPTDevice::extractSubdeviceFromDeviceID(id);

	return id & OMIT_SUBDEVICE_MASK;
}

int VPTDevice::extractSubdeviceFromDeviceID(const DeviceID& id)
{
	int zone = (id & EXTRACT_ZONE_MASK) >> 48;

	if (0 <= zone && zone <= 4)
		return zone;

	throw InvalidArgumentException("invalid subdevice number " + to_string(zone));
}

string VPTDevice::extractNonce(const string& response)
{
	RegularExpression re("var randnum = ([0-9]+)");
	RegularExpression::MatchVec matches;
	string nonce;

	if (re.match(response, 0, matches) != 0)
		nonce = response.substr(matches[1].offset, matches[1].length);

	return nonce;
}

string VPTDevice::generateHashPassword(const string& pwd, const string& random)
{
	SHA1Engine sha1;
	DigestOutputStream str(sha1);

	str << random + pwd;
	str.flush();

	const DigestEngine::Digest & digest = sha1.digest();
	return DigestEngine::digestToHex(digest);
}

vector<SensorData> VPTDevice::requestValues()
{
	HTTPRequest request;
	request.setMethod(HTTPRequest::HTTP_GET);
	request.setURI("/values.json");

	HTTPEntireResponse response = sendRequest(request, m_httpTimeout);

	stampVPT(Action::READ);

	VPTValuesParser parser;
	return parser.parse(m_deviceId, response.getBody());
}

vector<NewDeviceCommand::Ptr> VPTDevice::createNewDeviceCommands(Timespan& refresh)
{
	vector<NewDeviceCommand::Ptr> vector;

	std::list<ModuleType> zoneModules = VPTDevice::ZONE_MODULE_TYPES;
	for (int i = 1; i <= COUNT_OF_ZONES; i++) {
		vector.push_back(
			new NewDeviceCommand(
				VPTDevice::createSubdeviceID(i, m_deviceId),
				VPT_VENDOR,
				"Zone " + to_string(i),
				zoneModules,
				refresh));
	}

	std::list<ModuleType> boilerModules = VPTDevice::BOILER_MODULE_TYPES;
	vector.push_back(
		new NewDeviceCommand(
			m_deviceId,
			VPT_VENDOR,
			"Boiler",
			boilerModules,
			refresh));

	return vector;
}

HTTPEntireResponse VPTDevice::sendRequest(HTTPRequest& request, const Timespan& timeout) const
{
	HTTPEntireResponse response;

	logger().information("request: " + m_address.toString() + request.getURI(), __FILE__, __LINE__);

	response = HTTPUtil::makeRequest(
		request, m_address.host().toString(), m_address.port(), "", timeout);

	const int status = response.getStatus();
	if (status >= 400)
		logger().warning("response: " + to_string(status), __FILE__, __LINE__);
	else
		logger().information("response: " + to_string(status), __FILE__, __LINE__);

	return response;
}
