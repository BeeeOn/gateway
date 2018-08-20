#include <Poco/Exception.h>

#include "di/Injectable.h"
#include "zwave/ClimaxZWaveMapperRegistry.h"

BEEEON_OBJECT_BEGIN(BeeeOn, ClimaxZWaveMapperRegistry)
BEEEON_OBJECT_CASTABLE(ZWaveMapperRegistry)
BEEEON_OBJECT_PROPERTY("specMap", &ClimaxZWaveMapperRegistry::setSpecMap)
BEEEON_OBJECT_END(BeeeOn, ClimaxZWaveMapperRegistry)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

using CC = ZWaveNode::CommandClass;

ClimaxZWaveMapperRegistry::ClimaxZWaveMapperRegistry()
{
	registerInstantiator("dc23zw", new SimpleMapperInstantiator<DC23ZWMapper>);
}

#define DOOR_OPEN  1
#define DOOR_CLOSE 0
#define TAMPER     1

static const list<ModuleType> DC23ZW_TYPES = {
	{ModuleType::Type::TYPE_BATTERY},
	{ModuleType::Type::TYPE_SECURITY_ALERT},
	{ModuleType::Type::TYPE_OPEN_CLOSE},
};

list<ModuleType> ClimaxZWaveMapperRegistry::DC23ZWMapper::types() const
{
	return DC23ZW_TYPES;
}

ClimaxZWaveMapperRegistry::DC23ZWMapper::DC23ZWMapper(
		const ZWaveNode::Identity &id,
		const string &product):
	Mapper(id, product)
{
}

static bool isDoorOpen(const ZWaveNode::Value &value)
{
	switch (value.commandClass().id()) {
	case CC::ALARM:
		if (value.commandClass().index() == 6)
			return value.asInt() == 22;

		return false;

	case CC::SENSOR_BINARY:
		if (value.commandClass().index() == 10)
			return !value.asBool();

		return false;

	default:
		return false;
	}
}

static bool isDoorClosed(const ZWaveNode::Value &value)
{
	switch (value.commandClass().id()) {
	case CC::ALARM:
		if (value.commandClass().index() == 6)
			return value.asInt() == 23;

		return false;

	case CC::SENSOR_BINARY:
		if (value.commandClass().index() == 10)
			return value.asBool();

		return false;

	default:
		return false;
	}
}

SensorValue ClimaxZWaveMapperRegistry::DC23ZWMapper::convert(
		const ZWaveNode::Value &value) const
{
	const auto cc = value.commandClass().id();
	const auto index = value.commandClass().index();

	switch (cc) {
	case CC::BATTERY:
		return SensorValue(0, value.asDouble());

	case CC::ALARM:
		if (index == 7 && value.asInt() == 3)
			return SensorValue(1, TAMPER);
		if (index == 7 && value.asInt() == 0)
			return SensorValue(1, !TAMPER);

		break;
	}

	if (isDoorOpen(value))
		return SensorValue(2, DOOR_OPEN);
	if (isDoorClosed(value))
		return SensorValue(2, DOOR_CLOSE);

	throw InvalidArgumentException(
		"unrecognized value: " + value.toString());
}
