#include <Poco/DigestStream.h>
#include <Poco/Event.h>
#include <Poco/NumberFormatter.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/RegularExpression.h>
#include <Poco/SHA1Engine.h>
#include <Poco/StreamCopier.h>
#include <Poco/String.h>

#include "model/DevicePrefix.h"
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
	m_address(address),
	m_executor(*this)
{
}

VPTDevice::VPTDevice():
	m_executor(*this)
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

bool VPTDevice::operator==(const VPTDevice& other) const
{
	return other.deviceID() == m_deviceId;
}

VPTDevice::Ptr VPTDevice::buildDevice(const SocketAddress& address, const Timespan& timeout)
{
	VPTDevice::Ptr device = new VPTDevice(address);
	device->m_httpTimeout = timeout;
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
}

void VPTDevice::requestModifyState(const DeviceID& id, const ModuleID module,
	const double value, Result::Ptr result)
{
	const int zone = VPTDevice::extractSubdeviceFromDeviceID(id);
	if (zone == 0) {
		result->setStatus(Result::Status::FAILED);
		return;
	}

	switch (module.value()) {
	case VPTZoneModuleType::MOD_BOILER_OPERATION_TYPE:
		if (!m_executor.isRunning()) {
			try {
				m_executor.setZone(zone);
				m_executor.setValue(value);
				m_executor.setResult(result);
				m_executor.start();
			}
			catch (const Exception &e) {
				logger().log(e, __FILE__, __LINE__);
				logger().critical("command executor thread failed to start", __FILE__, __LINE__);
				result->setStatus(Result::Status::FAILED);
			}
		}
		else {
			logger().debug("command executor thread seems to be running already, dropping request to set",
				__FILE__, __LINE__);
			result->setStatus(Result::Status::FAILED);
		}
		break;
	case VPTZoneModuleType::MOD_BOILER_OPERATION_MODE:
		requestSetModBoilerOperationMode(zone, value, result);
		break;
	case VPTZoneModuleType::MANUAL_REQUESTED_ROOM_TEMPERATURE:
		requestSetManualRoomTemperature(zone, value, result);
		break;
	case VPTZoneModuleType::MANUAL_REQUESTED_WATER_TEMPERATURE:
		requestSetManualWaterTemperature(zone, value, result);
		break;
	case VPTZoneModuleType::MANUAL_REQUESTED_TUV_TEMPERATURE:
		requestSetManualTUVTemperature(zone, value, result);
		break;
	case VPTZoneModuleType::MOD_REQUESTED_WATER_TEMPERATURE_SET:
		requestSetModWaterTemperature(zone, value, result);
		break;
	}
}

void VPTDevice::requestSetModBoilerOperationType(const int zone, const int value, Result::Ptr result)
{
	string registr = REG_BOILER_OPER_TYPE.at(zone - 1);

	string strValue;
	for (auto item : VPTValuesParser::BOILER_OPERATION_TYPE) {
		if (item.second == value)
			strValue = item.first;
	}

	prepareAndSendRequest(registr, strValue);

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

		if (strValue == newValue) {
			result->setStatus(Result::Status::SUCCESS);
			return;
		}
	}

	result->setStatus(Result::Status::FAILED);
}

void VPTDevice::requestSetModBoilerOperationMode(const int zone, const int value, Result::Ptr result)
{
	string registr = REG_BOILER_OPER_MODE.at(zone - 1);

	string strValue;
	for (auto item : VPTValuesParser::BOILER_OPERATION_MODE) {
		if (item.second == value)
			strValue = item.first;
	}

	HTTPEntireResponse response = prepareAndSendRequest(registr, strValue);

	string newValue = parseZoneAttrFromJson(response.getBody(), zone,
			VPTZoneModuleType(VPTZoneModuleType::MOD_BOILER_OPERATION_MODE).toString());

	if (strValue == newValue)
		result->setStatus(Result::Status::SUCCESS);
	else
		result->setStatus(Result::Status::FAILED);
}

void VPTDevice::requestSetManualRoomTemperature(const int zone, const double value, Result::Ptr result)
{
	string registr = REG_MAN_ROOM_TEMP.at(zone - 1);
	string strValue = NumberFormatter::format(value, 1);
	strValue = replace(strValue, ".", "%2C");

	HTTPEntireResponse response = prepareAndSendRequest(registr, strValue);

	string newValue = parseZoneAttrFromJson(response.getBody(), zone,
			VPTZoneModuleType(VPTZoneModuleType::MANUAL_REQUESTED_ROOM_TEMPERATURE).toString());

	if (value == NumberParser::parseFloat(newValue, ',', '.'))
		result->setStatus(Result::Status::SUCCESS);
	else
		result->setStatus(Result::Status::FAILED);
}

void VPTDevice::requestSetManualWaterTemperature(const int zone, const double value, Result::Ptr result)
{
	string registr = REG_MAN_WATER_TEMP.at(zone - 1);
	string strValue = NumberFormatter::format(value, 0);

	HTTPEntireResponse response = prepareAndSendRequest(registr, strValue);

	string newValue = parseZoneAttrFromJson(response.getBody(), zone,
			VPTZoneModuleType(VPTZoneModuleType::MANUAL_REQUESTED_WATER_TEMPERATURE).toString());

	if (value == NumberParser::parseFloat(newValue, ',', '.'))
		result->setStatus(Result::Status::SUCCESS);
	else
		result->setStatus(Result::Status::FAILED);
}

void VPTDevice::requestSetManualTUVTemperature(const int zone, const double value, Result::Ptr result)
{
	string registr = REG_MAN_TUV_TEMP.at(zone - 1);
	string strValue = NumberFormatter::format(value, 0);

	HTTPEntireResponse response = prepareAndSendRequest(registr, strValue);

	string newValue = parseZoneAttrFromJson(response.getBody(), zone,
			VPTZoneModuleType(VPTZoneModuleType::MANUAL_REQUESTED_TUV_TEMPERATURE).toString());

	if (value == NumberParser::parseFloat(newValue, ',', '.'))
		result->setStatus(Result::Status::SUCCESS);
	else
		result->setStatus(Result::Status::FAILED);
}

void VPTDevice::requestSetModWaterTemperature(const int zone, const double value, Result::Ptr result)
{
	string registr = REG_MOD_WATER_TEMP.at(zone - 1);
	string strValue = NumberFormatter::format(value, 0);

	HTTPEntireResponse response = prepareAndSendRequest(registr, strValue);

	string newValue = parseZoneAttrFromJson(response.getBody(), zone,
			VPTZoneModuleType(VPTZoneModuleType::MOD_REQUESTED_WATER_TEMPERATURE_SET).toString());

	if (value == NumberParser::parseFloat(newValue, ',', '.'))
		result->setStatus(Result::Status::SUCCESS);
	else
		result->setStatus(Result::Status::FAILED);
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
		request.setURI(request.getURI() + "&__HOSTPWD=" +
			VPTDevice::generateHashPassword(m_password, VPTDevice::extractNonce(response.getBody())));
		response = sendRequest(request, m_httpTimeout);
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

	VPTValuesParser parser;
	return parser.parse(m_deviceId, response.getBody());
}

vector<NewDeviceCommand::Ptr> VPTDevice::createNewDeviceCommands()
{
	vector<NewDeviceCommand::Ptr> vector;

	std::list<ModuleType> zoneModules = VPTDevice::ZONE_MODULE_TYPES;
	for (int i = 1; i <= COUNT_OF_ZONES; i++) {
		vector.push_back(
			new NewDeviceCommand(
				VPTDevice::createSubdeviceID(i, m_deviceId),
				VPT_VENDOR,
				"Zone " + to_string(i),
				zoneModules));
	}

	std::list<ModuleType> boilerModules = VPTDevice::BOILER_MODULE_TYPES;
	vector.push_back(
		new NewDeviceCommand(
			m_deviceId,
			VPT_VENDOR,
			"Boiler",
			boilerModules));

	return vector;
}

HTTPEntireResponse VPTDevice::sendRequest(HTTPRequest& request, const Timespan& timeout) const
{
	HTTPClientSession http;
	HTTPEntireResponse response;

	http.setHost(m_address.host().toString());
	http.setPort(m_address.port());
	http.setTimeout(timeout);

	logger().information("request: " + m_address.toString() + request.getURI(), __FILE__, __LINE__);

	http.sendRequest(request);

	istream &input = http.receiveResponse(response);
	response.readBody(input);

	const int status = response.getStatus();
	if (status >= 400)
		logger().warning("response: " + to_string(status), __FILE__, __LINE__);
	else
		logger().information("response: " + to_string(status), __FILE__, __LINE__);

	return response;
}

VPTDevice::VPTCommandExecutor::VPTCommandExecutor(VPTDevice& parent):
	m_parent(parent),
	m_activity(this, &VPTDevice::VPTCommandExecutor::executeCommand)
{
}

VPTDevice::VPTCommandExecutor::~VPTCommandExecutor()
{
	m_activity.stop();
	m_activity.wait(SETTING_DELAY_MS);
}

void VPTDevice::VPTCommandExecutor::setZone(const int zone)
{
	m_zone = zone;
}

void VPTDevice::VPTCommandExecutor::setValue(const double value)
{
	m_value = value;
}

void VPTDevice::VPTCommandExecutor::setResult(const Result::Ptr result)
{
	m_result = result;
}

void VPTDevice::VPTCommandExecutor::start()
{
	m_activity.start();
}

void VPTDevice::VPTCommandExecutor::executeCommand()
{
	m_parent.requestSetModBoilerOperationType(m_zone, m_value, m_result);
}

bool VPTDevice::VPTCommandExecutor::isRunning()
{
	return m_activity.isRunning();
}
