#include <Poco/Exception.h>

#include "di/Injectable.h"
#include "zwave/AeotecZWaveMapperRegistry.h"

BEEEON_OBJECT_BEGIN(BeeeOn, AeotecZWaveMapperRegistry)
BEEEON_OBJECT_CASTABLE(ZWaveMapperRegistry)
BEEEON_OBJECT_PROPERTY("specMap", &AeotecZWaveMapperRegistry::setSpecMap)
BEEEON_OBJECT_END(BeeeOn, AeotecZWaveMapperRegistry)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

using CC = ZWaveNode::CommandClass;

AeotecZWaveMapperRegistry::AeotecZWaveMapperRegistry()
{
	registerInstantiator("zw100", new SimpleMapperInstantiator<ZW100Mapper>);
}

static const list<ModuleType> ZW100_TYPES = {
	{ModuleType::Type::TYPE_BATTERY},
	{ModuleType::Type::TYPE_TEMPERATURE, {ModuleType::Attribute::ATTR_INNER}},
	{ModuleType::Type::TYPE_LUMINANCE,   {ModuleType::Attribute::ATTR_INNER}},
	{ModuleType::Type::TYPE_HUMIDITY},
	{ModuleType::Type::TYPE_ULTRAVIOLET},
	{ModuleType::Type::TYPE_SHAKE},
};

list<ModuleType> AeotecZWaveMapperRegistry::ZW100Mapper::types() const
{
	return ZW100_TYPES;
}

AeotecZWaveMapperRegistry::ZW100Mapper::ZW100Mapper(
		const ZWaveNode::Identity &id,
		const string &product):
	Mapper(id, product)
{
}

SensorValue AeotecZWaveMapperRegistry::ZW100Mapper::convert(
		const ZWaveNode::Value &value) const
{
	switch (value.commandClass().id()) {
	case CC::BATTERY:
		return SensorValue(0, value.asDouble());

	case CC::SENSOR_MULTILEVEL:
		switch (value.commandClass().index()) {
		case 0x01:
			return SensorValue(1, value.asCelsius());
		case 0x03:
			return SensorValue(2, value.asLuminance());
		case 0x05:
			return SensorValue(3, value.asDouble());
		case 0x1B:
			return SensorValue(4, value.asDouble());
		}
		break;

	case CC::ALARM:
		if (value.asDouble() == 0x03)
			return SensorValue(5, 1);
		break;

	case CC::SENSOR_BINARY: // TODO: sometimes reported but what is it?
		break;
	}

	throw InvalidArgumentException(
		"unrecognized value: " + value.toString());
}
