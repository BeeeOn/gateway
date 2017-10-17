#include <Poco/NumberParser.h>
#include <Poco/StringTokenizer.h>

#include "jablotron/JablotronDeviceTP82N.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const ModuleID MODULE_ID_CURRENT_ROOM_TEMPERATURE = 0;
static const ModuleID MODULE_ID_REQUESTED_ROOM_TEMPERATURE = 1;
static const ModuleID MODULE_ID_BATTERY_LEVEL = 2;

static const set<ModuleType::Attribute> CURRENT_TEMPERATURE_ATTR = {
	ModuleType::Attribute::TYPE_INNER,
};
static const set<ModuleType::Attribute> REQUESTED_TEMPERATURE_ATTR = {
	ModuleType::Attribute::TYPE_INNER,
	ModuleType::Attribute::TYPE_MANUAL_ONLY,
	ModuleType::Attribute::TYPE_CONTROLLABLE,
};

static const list<ModuleType> MODULE_TYPES = {
	ModuleType(
		ModuleType::Type::TYPE_TEMPERATURE,
		REQUESTED_TEMPERATURE_ATTR),
	ModuleType(
		ModuleType::Type::TYPE_TEMPERATURE,
		CURRENT_TEMPERATURE_ATTR),
	ModuleType(ModuleType::Type::TYPE_BATTERY)
};

JablotronDeviceTP82N::JablotronDeviceTP82N(const DeviceID &deviceID):
	JablotronDevice(deviceID, "TP-82N")
{
}

SensorData JablotronDeviceTP82N::extractSensorData(const string &message)
{
	StringTokenizer tokens(message, " ");

	SensorData sensorData;
	sensorData.setDeviceID(deviceID());

	StringTokenizer tokenIntSet(tokens[2], ":");
	string temperature = tokenIntSet[1].substr(0, 4);

	if (tokenIntSet[0] == "SET") {
		sensorData.insertValue(
			SensorValue(
				ModuleID(MODULE_ID_REQUESTED_ROOM_TEMPERATURE),
				NumberParser::parseFloat(temperature)
			)
		);
	}
	else if (tokenIntSet[0] == "INT") {
		sensorData.insertValue(
			SensorValue(
				ModuleID(MODULE_ID_CURRENT_ROOM_TEMPERATURE),
				NumberParser::parseFloat(temperature)
			)
		);
	}
	else {
		throw InvalidArgumentException("unexpected message: " + message);
	}

	sensorData.insertValue(
		SensorValue(
			ModuleID(MODULE_ID_BATTERY_LEVEL),
			extractBatteryLevel(tokens[3])
		)
	);

	return sensorData;
}

list<ModuleType> JablotronDeviceTP82N::moduleTypes()
{
	return MODULE_TYPES;
}
