#include <Poco/JSON/Object.h>
#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>

#include "conrad/RadiatorThermostat.h"
#include "util/JsonUtil.h"

#define CURRENT_TEMPERATURE_MODULE_ID 0
#define DESIRED_TEMPERATURE_MODULE_ID 1
#define VALVE_POSITION_MODULE_ID 2
#define RSSI_MODULE_ID 3

using namespace BeeeOn;
using namespace Poco::JSON;
using namespace Poco;
using namespace std;

static list<ModuleType> DEVICE_MODULE_TYPES = {
	{ModuleType::Type::TYPE_TEMPERATURE},
	{ModuleType::Type::TYPE_TEMPERATURE},
	{ModuleType::Type::TYPE_OPEN_RATIO},
	{ModuleType::Type::TYPE_RSSI},
};

const string RadiatorThermostat::PRODUCT_NAME = "HM-CC-RT-DN";

RadiatorThermostat::RadiatorThermostat(
		const DeviceID& id,
		const RefreshTime &refresh):
	ConradDevice(id, refresh, PRODUCT_NAME, DEVICE_MODULE_TYPES)
{
}

RadiatorThermostat::~RadiatorThermostat()
{
}

/**
 * Message example:
 * {
 *     "channels" : {
 *         "Clima" : "T: 21.2 desired: 17.0 valve: 0",
 *         "ClimaTeam" : "unpeered",
 *         "Climate" : "unpeered",
 *         "Main" : "CMDs_done",
 *         "Weather" : "21.2",
 *         "WindowRec" : "last:trigLast",
 *         "remote" : "unpeered"
 *     },
 *     "dev" : "HM_36BA59",
 *     "event" : "message",
 *     "model" : "HM-CC-RT-DN",
 *     "raw" : "A0FE0861036BA590000000A88D40C0000",
 *     "rssi" : -41.5,
 *     "serial" : "MEQ0233325",
 *     "type" : "thermostat"
 *  }
 */
SensorData RadiatorThermostat::parseMessage(const Object::Ptr message)
{
	SensorData data;
	Poco::RegularExpression::MatchVec matches;

	data.setDeviceID(m_deviceId);
	Object::Ptr object = message->getObject("channels");

	Poco::RegularExpression re("T: ([+-]?[0-9]+(\\.[0-9]+)?) desired: ([+-]?[0-9]+(\\.[0-9]+)?) valve: (0|1)");

	string str = object->getValue<std::string>("Clima");
	if (re.match(str, 0, matches) == 0)
		throw IllegalStateException("cannot parse Radiator Thermostat message");

	string current = str.substr(matches[1].offset, matches[1].length);
	string desired = str.substr(matches[3].offset, matches[3].length);
	string valvePosition = str.substr(matches[5].offset, matches[5].length);

	data.insertValue(SensorValue(CURRENT_TEMPERATURE_MODULE_ID, NumberParser::parseFloat(current)));
	data.insertValue(SensorValue(DESIRED_TEMPERATURE_MODULE_ID, NumberParser::parseFloat(desired)));
	data.insertValue(SensorValue(VALVE_POSITION_MODULE_ID, NumberParser::parseUnsigned(valvePosition)));
	data.insertValue(SensorValue(RSSI_MODULE_ID, message->getValue<double>("rssi")));

	return data;
}
