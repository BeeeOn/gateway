#include <Poco/NumberParser.h>
#include <Poco/StringTokenizer.h>

#include "JablotronDeviceRC86K.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const ModuleID MODULE_ID_OPEN_CLOSE = 0;
static const ModuleID MODULE_ID_SECURITY_ALERT = 1;
static const ModuleID MODULE_ID_BATTERY_LEVEL = 2;

static const int PANIC_VALUE = 1;

static const set<ModuleType::Attribute> REQUESTED_TEMPERATURE_ATTR = {
	ModuleType::Attribute::TYPE_MANUAL_ONLY,
	ModuleType::Attribute::TYPE_CONTROLLABLE,
};

static const list<ModuleType> MODULE_TYPES = {
	ModuleType(
		ModuleType::Type::TYPE_OPEN_CLOSE,
		REQUESTED_TEMPERATURE_ATTR),
	ModuleType(ModuleType::Type::TYPE_SECURITY_ALERT),
	ModuleType(ModuleType::Type::TYPE_BATTERY),
};

JablotronDeviceRC86K::JablotronDeviceRC86K(
		const DeviceID &deviceID, const string &name):
	JablotronDevice(deviceID, name)
{
}

SensorData JablotronDeviceRC86K::extractSensorData(const string &message)
{
	StringTokenizer tokens(message, " ");

	SensorData sensorData;
	sensorData.setDeviceID(deviceID());

	if (tokens[2] == "PANIC") {
		sensorData.insertValue(
			SensorValue(
				ModuleID(MODULE_ID_SECURITY_ALERT),
				PANIC_VALUE
			)
		);
	}
	else {
		sensorData.insertValue(
			SensorValue(
				ModuleID(MODULE_ID_OPEN_CLOSE),
				parseValue(tokens[2])
			)
		);
	}

	sensorData.insertValue(
		SensorValue(
			ModuleID(MODULE_ID_BATTERY_LEVEL),
			extractBatteryLevel(tokens[3])
		)
	);

	return sensorData;
}

list<ModuleType> JablotronDeviceRC86K::moduleTypes()
{
	return MODULE_TYPES;
}
