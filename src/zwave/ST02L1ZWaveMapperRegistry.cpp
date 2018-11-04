#include <Poco/Exception.h>

#include "di/Injectable.h"
#include "zwave/ST02L1ZWaveMapperRegistry.h"

BEEEON_OBJECT_BEGIN(BeeeOn, ST02L1ZWaveMapperRegistry)
BEEEON_OBJECT_CASTABLE(ZWaveMapperRegistry)
BEEEON_OBJECT_PROPERTY("specMap", &ST02L1ZWaveMapperRegistry::setSpecMap)
BEEEON_OBJECT_END(BeeeOn, ST02L1ZWaveMapperRegistry)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

using CC = ZWaveNode::CommandClass;

ST02L1ZWaveMapperRegistry::ST02L1ZWaveMapperRegistry()
{
	registerInstantiator("4-in-1", new SimpleMapperInstantiator<Device4in1Mapper>);
	registerInstantiator("3-in-1-pir", new SimpleMapperInstantiator<Device3in1WithPIRMapper>);
	registerInstantiator("3-in-1", new SimpleMapperInstantiator<Device3in1Mapper>);
}

#define DOOR_OPEN   1
#define DOOR_CLOSED 0
#define TAMPER      1
#define MOTION      1

SensorValue ST02L1ZWaveMapperRegistry::convert(const ZWaveNode::Value &value)
{
	const auto cc = value.commandClass().id();
	const auto index = value.commandClass().index();

	switch (cc) {
	case CC::BATTERY:
		return SensorValue(0, value.asDouble());

	case CC::SENSOR_MULTILEVEL:
		switch (index) {
		case 0x01:
			return SensorValue(1, value.asCelsius());
		case 0x03:
			return SensorValue(2, value.asLuminance());
		}
		break;

	case CC::ALARM:
		if (index == 6 && value.asInt() == 254)
			return SensorValue(3, TAMPER);
		if (index == 7 && value.asInt() == 3)
			return SensorValue(3, TAMPER);

		break;

	case CC::SENSOR_BINARY:
		switch (index) {
		case 0:
		case 8:
			return SensorValue(3, value.asBool() ? TAMPER : 0);
		}

		break;

	default:
		break;
	}

	throw InvalidArgumentException(
		"unrecognized value: " + value.toString());
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

static bool isMotionDetected(const ZWaveNode::Value &value)
{
	switch (value.commandClass().id()) {
	case CC::ALARM:
		if (value.commandClass().index() == 7)
			return value.asInt() == 8;

		return false;

	case CC::SENSOR_BINARY:
		if (value.commandClass().index() == 12)
			return value.asBool();

		return false;

	default:
		return false;
	}
}

static bool isMotionNotDetected(const ZWaveNode::Value &value)
{
	switch (value.commandClass().id()) {
	case CC::SENSOR_BINARY:
		if (value.commandClass().index() == 12)
			return !value.asBool();

		return false;

	default:
		return false;
	}
}

ST02L1ZWaveMapperRegistry::Device3in1Mapper::Device3in1Mapper(
		const ZWaveNode::Identity &id,
		const std::string &product,
		bool pirVariant):
	Mapper(id, product),
	m_pirVariant(pirVariant)
{
}

list<ModuleType> ST02L1ZWaveMapperRegistry::Device3in1Mapper::types() const
{
	const ModuleType motion = {ModuleType::Type::TYPE_MOTION};
	const ModuleType door = {ModuleType::Type::TYPE_OPEN_CLOSE};

	return {
		{ModuleType::Type::TYPE_BATTERY},
		{ModuleType::Type::TYPE_TEMPERATURE, {ModuleType::Attribute::ATTR_INNER}},
		{ModuleType::Type::TYPE_LUMINANCE,   {ModuleType::Attribute::ATTR_INNER}},
		{ModuleType::Type::TYPE_SECURITY_ALERT},
		m_pirVariant? motion : door
	};
}

SensorValue ST02L1ZWaveMapperRegistry::Device3in1Mapper::convert(
		const ZWaveNode::Value &value) const
{
	if (m_pirVariant) {
		if (isMotionDetected(value))
			return SensorValue(4, MOTION);
		if (isMotionNotDetected(value))
			return SensorValue(4, 0);
	}
	else {
		if (isDoorOpen(value))
			return SensorValue(4, DOOR_OPEN);
		if (isDoorClosed(value))
			return SensorValue(4, DOOR_CLOSED);
	}

	return ST02L1ZWaveMapperRegistry::convert(value);
}

ST02L1ZWaveMapperRegistry::Device3in1WithPIRMapper::Device3in1WithPIRMapper(
		const ZWaveNode::Identity &id,
		const std::string &product):
	Device3in1Mapper(id, product, true)
{
}

ST02L1ZWaveMapperRegistry::Device4in1Mapper::Device4in1Mapper(
		const ZWaveNode::Identity &id,
		const std::string &product):
	Mapper(id, product)
{
}

list<ModuleType> ST02L1ZWaveMapperRegistry::Device4in1Mapper::types() const
{
	return {
		{ModuleType::Type::TYPE_BATTERY},
		{ModuleType::Type::TYPE_TEMPERATURE, {ModuleType::Attribute::ATTR_INNER}},
		{ModuleType::Type::TYPE_LUMINANCE,   {ModuleType::Attribute::ATTR_INNER}},
		{ModuleType::Type::TYPE_SECURITY_ALERT},
		{ModuleType::Type::TYPE_OPEN_CLOSE},
		{ModuleType::Type::TYPE_MOTION},
	};
}

SensorValue ST02L1ZWaveMapperRegistry::Device4in1Mapper::convert(
		const ZWaveNode::Value &value) const
{
	if (isDoorOpen(value))
		return SensorValue(4, DOOR_OPEN);
	if (isDoorClosed(value))
		return SensorValue(4, DOOR_CLOSED);
	if (isMotionDetected(value))
		return SensorValue(5, MOTION);
	if (isMotionNotDetected(value))
		return SensorValue(5, 0);

	return ST02L1ZWaveMapperRegistry::convert(value);
}
