#include <Poco/NumberParser.h>
#include <Poco/StringTokenizer.h>

#include "jablotron/JablotronDeviceJA85ST.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const ModuleID MODULE_ID_FIRE_SENSOR_ALARM = 0;
static const ModuleID MODULE_ID_FIRE_SECURITY_ALERT = 1;
static const ModuleID MODULE_ID_BATTERY_STATE = 2;

static const int SENSOR_VALUE = 1;

static const list<ModuleType> MODULE_TYPES = {
	ModuleType(ModuleType::Type::TYPE_FIRE),
	ModuleType(ModuleType::Type::TYPE_SECURITY_ALERT),
	ModuleType(ModuleType::Type::TYPE_BATTERY),
};

JablotronDeviceJA85ST::JablotronDeviceJA85ST(
		const DeviceID &deviceID, const string &name):
	JablotronDevice(deviceID, name)
{
}

SensorData JablotronDeviceJA85ST::extractSensorData(const string &message)
{
	StringTokenizer tokens(message, " ");

	SensorData sensorData;
	sensorData.setDeviceID(deviceID());

	if (tokens[2] == "SENSOR") {
		sensorData.insertValue(
			SensorValue(
				ModuleID(MODULE_ID_FIRE_SENSOR_ALARM),
				SENSOR_VALUE
			)
		);
	}
	else if (tokens[2] == "TAMPER") {
		sensorData.insertValue(
			SensorValue(
				ModuleID(MODULE_ID_FIRE_SECURITY_ALERT),
				parseValue(tokens[4])
			)
		);
	}
	else if (tokens[2] != "BEACON") {
		throw InvalidArgumentException("unexpected message: " + message);
	}

	sensorData.insertValue(
		SensorValue(
			ModuleID(MODULE_ID_BATTERY_STATE),
			extractBatteryLevel(tokens[3])
		)
	);

	return sensorData;
}

list<ModuleType> JablotronDeviceJA85ST::moduleTypes()
{
	return MODULE_TYPES;
}

Timespan JablotronDeviceJA85ST::refreshTime()
{
	return REFRESH_TIME_SUPPORTED_BEACON;
}
