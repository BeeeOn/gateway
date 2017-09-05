#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/NumberFormatter.h>

#include "model/SensorValue.h"
#include "vpt/VPTBoilerModuleType.h"
#include "vpt/VPTDevice.h"
#include "vpt/VPTValuesParser.h"
#include "vpt/VPTZoneModuleType.h"
#include "util/JsonUtil.h"

#define BAR_TO_HECTOPASCALS(bar) (bar * 1000)
#define BOILER_OPERATION_TYPE_OFF "Vypnuto_"
#define BOILER_OPERATION_TYPE_ROOM_REGULATOR "Pok.ter."
#define BOILER_OPERATION_TYPE_EQUITERM_REGULATOR "Ekviterm"
#define BOILER_OPERATION_TYPE_CONSTANT_WATER_TEMPERATURE "Tep.vody"
#define BOILER_OPERATION_TYPE_HOT_WATER "OhrevTUV"
#define BOLER_OPERATION_MODE_AUTOMATIC "Cas.prog."
#define BOLER_OPERATION_MODE_MANUAL "Rucne____"
#define BOLER_OPERATION_MODE_VACATION "Dovolena_"
#define BOILER_STATUS_UNDEFINED "OFF LINE "
#define BOILER_STATUS_HEATING "PROVOZ   "
#define BOILER_STATUS_HOT_WATER "OHREV TUV"
#define BOILER_STATUS_FAILURE "PORUCHA  "
#define BOILER_STATUS_SHUTDOWN "ODSTAVENO"
#define BOILER_MODE_UNDEFINED ""
#define BOILER_MODE_ON "ZAPNUTO "
#define BOILER_MODE_OFF "VYPNUTO "

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::JSON;
using namespace std;

const map<string, int> VPTValuesParser::BOILER_OPERATION_TYPE =
	{{BOILER_OPERATION_TYPE_OFF, 0},
	{BOILER_OPERATION_TYPE_ROOM_REGULATOR, 1},
	{BOILER_OPERATION_TYPE_EQUITERM_REGULATOR, 2},
	{BOILER_OPERATION_TYPE_CONSTANT_WATER_TEMPERATURE, 3},
	{BOILER_OPERATION_TYPE_HOT_WATER, 4}};

const map<string, int> VPTValuesParser::BOILER_OPERATION_MODE =
	{{BOLER_OPERATION_MODE_AUTOMATIC, 0},
	{BOLER_OPERATION_MODE_MANUAL, 1},
	{BOLER_OPERATION_MODE_VACATION, 2}};

const map<string, int> VPTValuesParser::BOILER_STATUS =
	{{BOILER_STATUS_UNDEFINED, 0},
	{BOILER_STATUS_HEATING, 1},
	{BOILER_STATUS_HOT_WATER, 2},
	{BOILER_STATUS_FAILURE, 3},
	{BOILER_STATUS_SHUTDOWN, 4}};

const map<string, int> VPTValuesParser::BOILER_MODE =
	{{BOILER_MODE_UNDEFINED, 0},
	{BOILER_MODE_ON, 1},
	{BOILER_MODE_OFF, 2}};

VPTValuesParser::VPTValuesParser()
{
}

VPTValuesParser::~VPTValuesParser()
{
}

vector<SensorData> VPTValuesParser::parse(const DeviceID& id, const string& content)
{
	vector<SensorData> list;

	Object::Ptr object = JsonUtil::parse(content);
	Object::Ptr sensors = object->getObject("sensors");

	for (int i = 1; i <= VPTDevice::COUNT_OF_ZONES; i++)
		list.push_back(parseZone(i, id, sensors));

	list.push_back(parseBoiler(id, sensors));

	return list;
}

SensorData VPTValuesParser::parseZone(const uint64_t zone, const DeviceID& id, const Object::Ptr json) const
{
	SensorData data;
	string strZone = "ZONE_" + to_string(zone);
	Object::Ptr sensor = json->getObject(strZone);

	data.setDeviceID(VPTDevice::createSubdeviceID(zone, id));

	string value;
	for (auto type : VPTZoneModuleType::valueMap()) {
		try {
			value = sensor->getValue<string>(type.second);
		}
		catch (Exception& e) {
			logger().warning("can not find " + type.second, __FILE__, __LINE__);
			continue;
		}

		try {
			switch (type.first) {
			case VPTZoneModuleType::MOD_BOILER_OPERATION_TYPE:
				data.insertValue(SensorValue(type.first, BOILER_OPERATION_TYPE.at(value)));
				break;
			case VPTZoneModuleType::MOD_BOILER_OPERATION_MODE:
				data.insertValue(SensorValue(type.first, BOILER_OPERATION_MODE.at(value)));
				break;
			case VPTZoneModuleType::MOD_REQUESTED_ROOM_TEMPERATURE:
				data.insertValue(SensorValue(type.first, NumberParser::parseFloat(value, ',', '.')));
				break;
			case VPTZoneModuleType::MOD_CURRENT_ROOM_TEMPERATURE:
				data.insertValue(SensorValue(type.first, NumberParser::parseFloat(value, ',', '.')));
				break;
			case VPTZoneModuleType::MOD_CURRENT_WATER_TEMPERATURE:
				data.insertValue(SensorValue(type.first, NumberParser::parse(value)));
				break;
			default:
				break;
			}
		}
		catch (SyntaxException& e) {
			data.insertValue(SensorValue(type.first));
			logger().warning("can not retrieve data " + type.second, __FILE__, __LINE__);
		}
	}

	return data;
}

SensorData VPTValuesParser::parseBoiler(const DeviceID& id, const Object::Ptr json) const
{
	SensorData data;
	Object::Ptr sensor = json->getObject("BOILER");

	data.setDeviceID(id);

	string value;
	for (auto type : VPTBoilerModuleType::valueMap()) {
		try {
			value = sensor->getValue<string>(type.second);
		}
		catch (Exception& e) {
			logger().warning("can not find " + type.second, __FILE__, __LINE__);
			continue;
		}

		try {
			switch (type.first) {
			case VPTBoilerModuleType::MOD_BOILER_STATUS:
				data.insertValue(SensorValue(type.first, BOILER_STATUS.at(value)));
				break;
			case VPTBoilerModuleType::MOD_BOILER_MODE:
				data.insertValue(SensorValue(type.first, BOILER_MODE.at(value)));
				break;
			case VPTBoilerModuleType::MOD_CURRENT_WATER_TEMPERATURE:
				data.insertValue(SensorValue(type.first, NumberParser::parse(value)));
				break;
			case VPTBoilerModuleType::MOD_CURRENT_OUTSIDE_TEMPERATURE:
				data.insertValue(SensorValue(type.first, NumberParser::parseFloat(value, ',', '.')));
				break;
			case VPTBoilerModuleType::MOD_AVERAGE_OUTSIDE_TEMPERATURE:
				data.insertValue(SensorValue(type.first, NumberParser::parseFloat(value, ',', '.')));
				break;
			case VPTBoilerModuleType::MOD_CURRENT_BOILER_PERFORMANCE:
				data.insertValue(SensorValue(type.first, NumberParser::parse(value)));
				break;
			case VPTBoilerModuleType::MOD_CURRENT_BOILER_PRESSURE:
				data.insertValue(SensorValue(type.first, BAR_TO_HECTOPASCALS(NumberParser::parseFloat(value, ',', '.'))));
				break;
			case VPTBoilerModuleType::MOD_CURRENT_BOILER_ERROR:
				data.insertValue(SensorValue(type.first, NumberParser::parseFloat(value, ',', '.')));
				break;
			default:
				break;
			}
		}
		catch (SyntaxException& e) {
			data.insertValue(SensorValue(type.first));
			logger().warning("can not retrieve data " + type.second, __FILE__, __LINE__);
		}
	}

	return data;
}
