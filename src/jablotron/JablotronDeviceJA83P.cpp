#include <Poco/NumberParser.h>
#include <Poco/StringTokenizer.h>

#include "jablotron/JablotronDeviceJA83P.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const ModuleID MODULE_ID_SENSOR = 0;
static const ModuleID MODULE_ID_SECURITY_ALERT = 1;
static const ModuleID MODULE_ID_BATTERY_LEVEL = 2;

static const int SENSOR_VALUE = 1;

static const list<ModuleType> MODULE_TYPES = {
	ModuleType(ModuleType::Type::TYPE_MOTION),
	ModuleType(ModuleType::Type::TYPE_SECURITY_ALERT),
	ModuleType(ModuleType::Type::TYPE_BATTERY),
};

JablotronDeviceJA83P::JablotronDeviceJA83P(const DeviceID &deviceID):
	JablotronDevice(deviceID, "JA-83P")
{
}

SensorData JablotronDeviceJA83P::extractSensorData(const string &message)
{
	StringTokenizer tokens(message, " ");

	SensorData sensorData;
	sensorData.setDeviceID(deviceID());

	if (tokens[2] == "SENSOR") {
		sensorData.insertValue(
			SensorValue(
				ModuleID(MODULE_ID_SENSOR),
				SENSOR_VALUE
			)
		);
	}
	else if (tokens[2] == "TAMPER") {
		sensorData.insertValue(
			SensorValue(
				ModuleID(MODULE_ID_SECURITY_ALERT),
				parseValue(tokens[4])
			)
		);
	}
	else if (tokens[2] != "BEACON") {
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

list<ModuleType> JablotronDeviceJA83P::moduleTypes()
{
	return MODULE_TYPES;
}

Timespan JablotronDeviceJA83P::refreshTime()
{
	return REFRESH_TIME_SUPPORTED_BEACON;
}
