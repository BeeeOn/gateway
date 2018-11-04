#include <Poco/Exception.h>

#include "di/Injectable.h"
#include "zwave/FibaroZWaveMapperRegistry.h"

BEEEON_OBJECT_BEGIN(BeeeOn, FibaroZWaveMapperRegistry)
BEEEON_OBJECT_CASTABLE(ZWaveMapperRegistry)
BEEEON_OBJECT_PROPERTY("specMap", &FibaroZWaveMapperRegistry::setSpecMap)
BEEEON_OBJECT_END(BeeeOn, FibaroZWaveMapperRegistry)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

using CC = ZWaveNode::CommandClass;

FibaroZWaveMapperRegistry::FibaroZWaveMapperRegistry()
{
	registerInstantiator("fgk101", new SimpleMapperInstantiator<FGK101Mapper>);
	registerInstantiator("fgsd002", new SimpleMapperInstantiator<FGSD002Mapper>);
}

#define FGK101_DOOR_OPEN  1
#define FGK101_DOOR_CLOSE 0

static const list<ModuleType> FGK101_TYPES = {
	{ModuleType::Type::TYPE_BATTERY},
	{ModuleType::Type::TYPE_OPEN_CLOSE},
};

list<ModuleType> FibaroZWaveMapperRegistry::FGK101Mapper::types() const
{
	return FGK101_TYPES;
}

FibaroZWaveMapperRegistry::FGK101Mapper::FGK101Mapper(
		const ZWaveNode::Identity &id,
		const string &product):
	Mapper(id, product)
{
}

SensorValue FibaroZWaveMapperRegistry::FGK101Mapper::convert(
		const ZWaveNode::Value &value) const
{
	switch (value.commandClass().id()) {
	case CC::BATTERY:
		return SensorValue(0, value.asDouble());

	case CC::SENSOR_BINARY:
		return SensorValue(1, value.asBool());
	}

	throw InvalidArgumentException(
		"unrecognized value: " + value.toString());
}

static const list<ModuleType> FGSD002_TYPES = {
	{ModuleType::Type::TYPE_BATTERY},
	{ModuleType::Type::TYPE_TEMPERATURE},
	{ModuleType::Type::TYPE_SECURITY_ALERT},
	{ModuleType::Type::TYPE_SMOKE},
	{ModuleType::Type::TYPE_HEAT},
};


FibaroZWaveMapperRegistry::FGSD002Mapper::FGSD002Mapper(
		const ZWaveNode::Identity &id,
		const string &product):
	Mapper(id, product)
{
}

list<ModuleType> FibaroZWaveMapperRegistry::FGSD002Mapper::types() const
{
	return FGSD002_TYPES;
}

static SensorValue convertAlarm(const ZWaveNode::Value &value)
{
	const auto index = value.commandClass().index();
	const auto noEvent = value.asInt() == 254;

	if (value.commandClass().id() == CC::ALARM) {
		switch (index) {
		case 0x01: // smoke
			return SensorValue(3, !noEvent);
		case 0x04: // heat
			return SensorValue(4, !noEvent);
		case 0x07: // tampering
			return SensorValue(2, !noEvent);
		}
	}

	throw InvalidArgumentException(
		"unrecognized value: " + value.toString());
}

SensorValue FibaroZWaveMapperRegistry::FGSD002Mapper::convert(
		const ZWaveNode::Value &value) const
{
	switch (value.commandClass().id()) {
	case CC::BATTERY:
		return SensorValue(0, value.asDouble());

	case CC::SENSOR_MULTILEVEL:
		return SensorValue(1, value.asCelsius());
	}

	return convertAlarm(value);
}
