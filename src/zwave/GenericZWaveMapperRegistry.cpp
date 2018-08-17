#include <map>
#include <vector>

#include <Poco/AutoPtr.h>
#include <Poco/Exception.h>
#include <Poco/FileStream.h>
#include <Poco/Logger.h>
#include <Poco/NumberParser.h>
#include <Poco/String.h>

#include "di/Injectable.h"
#include "util/SecureXmlParser.h"
#include "zwave/GenericZWaveMapperRegistry.h"
#include "zwave/ZWaveNode.h"
#include "zwave/ZWaveTypeMappingParser.h"

BEEEON_OBJECT_BEGIN(BeeeOn, GenericZWaveMapperRegistry)
BEEEON_OBJECT_CASTABLE(ZWaveMapperRegistry)
BEEEON_OBJECT_PROPERTY("typesMapping", &GenericZWaveMapperRegistry::loadTypesMapping)
BEEEON_OBJECT_END(BeeeOn, GenericZWaveMapperRegistry)

using namespace std;
using namespace Poco;
using namespace Poco::XML;
using namespace BeeeOn;

using CC = ZWaveNode::CommandClass;

uint8_t GenericZWaveMapperRegistry::GenericMapper::ID_MANGLE_BITS = ~0;

GenericZWaveMapperRegistry::GenericMapper::GenericMapper(
		const ZWaveNode::Identity &id,
		const std::string &product):
	Mapper(id, product)
{
}

DeviceID GenericZWaveMapperRegistry::GenericMapper::buildID() const
{
	return mangleID(Mapper::buildID(), ID_MANGLE_BITS);
}

string GenericZWaveMapperRegistry::GenericMapper::product() const
{
	return Mapper::product() + " (generic)";
}

void GenericZWaveMapperRegistry::GenericMapper::mapType(
	const ZWaveNode::CommandClass &cc,
	const ModuleType &type)
{
	ModuleID id;

	if (!m_modules.empty()) {
		const ModuleID &last = m_modules.rbegin()->first;
		id = last.value() + 1;
	}

	m_mapping.emplace(cc, id);
	m_modules.emplace(id, type);
}

list<ModuleType> GenericZWaveMapperRegistry::GenericMapper::types() const
{
	list<ModuleType> types;

	for (const auto &pair : m_modules)
		types.emplace_back(pair.second);

	return types;
}

void GenericZWaveMapperRegistry::GenericMapper::cannotConvert(
		const ZWaveNode::Value &value) const
{
	throw InvalidArgumentException(
		"value " + value.toString() + " has no conversion method");
}

SensorValue GenericZWaveMapperRegistry::GenericMapper::convert(
		const ZWaveNode::Value &value) const
{
	auto it = m_mapping.find(value.commandClass());
	if (it == m_mapping.end()) {
		throw InvalidArgumentException(
			"unsupported command class "
			+ value.commandClass().toString());
	}

	double result;

	switch (value.commandClass().id()) {
	case CC::SWITCH_BINARY:
		result = value.asBool()? 1 : 0;
		break;

	case CC::SENSOR_BINARY:
		switch (value.commandClass().index()) {
		case 0x0A:
			result = value.asBool()? 0 : 1;
			break;

		default:
			result = value.asBool()? 1 : 0;
			break;

		}
		break;

	case CC::ALARM:
		switch (value.commandClass().index()) {
		case 0x01:
		case 0x02:
		case 0x03:
			if (value.asInt() == 1 || value.asInt() == 2)
				result = 1;
			else if (value.asInt() == 254)
				result = 1;
			else
				cannotConvert(value);
			break;

		case 0x04:
		case 0x07:
			if (value.asInt() >= 1 || value.asInt() <= 6)
				result = 1;
			else if (value.asInt() == 254)
				result = 1;
			else
				cannotConvert(value);
			break;

		case 0x05:
			if (value.asInt() >= 1 || value.asInt() <= 4)
				result = 1;
			else if (value.asInt() == 254)
				result = 1;
			else
				cannotConvert(value);
			break;

		case 0x08:
			if (value.asInt() == 1 || value.asInt() == 3)
				result = 1;
			else if (value.asInt() == 2)
				result = 0;
			else
				cannotConvert(value);
			break;

		case 0x09:
			if (value.asInt() == 1 || value.asInt() == 2)
				result = 1;
			else if (value.asInt() == 254)
				result = 1;
			else
				cannotConvert(value);
			break;

		case 0x0A:
			if (value.asInt() >= 1 || value.asInt() <= 3)
				result = 1;
			else if (value.asInt() == 254)
				result = 1;
			else
				cannotConvert(value);
			break;

		case 0x0B:
			if (value.asInt() == 1)
				result = 1;
			else
				cannotConvert(value);
			break;

		default:
			cannotConvert(value);
			break;
		}
		break;

	case CC::BATTERY:
		result = value.asDouble();
		break;

	case CC::SENSOR_MULTILEVEL:
		switch (value.commandClass().index()) {
		case 0x01:
		case 0x17:
		case 0x18:
		case 0x40:
			result = value.asCelsius();
			break;

		case 0x23:
			result = value.asPM25();
			break;

		default:
			result = value.asDouble();
			break;
		}

		break;

	default:
		cannotConvert(value);
		break;
	}

	return SensorValue{it->second, result};
}

GenericZWaveMapperRegistry::GenericZWaveMapperRegistry()
{
	m_typesMapping = {
		{{CC::BASIC,             0x00}, {ModuleType::Type::TYPE_UNKNOWN}},
		{{CC::SWITCH_BINARY,     0x00}, {ModuleType::Type::TYPE_ON_OFF,
							{ModuleType::Attribute::ATTR_CONTROLLABLE}}},
		{{CC::SENSOR_BINARY,     0x00}, {ModuleType::Type::TYPE_ON_OFF}}, // reserved
		{{CC::SENSOR_BINARY,     0x01}, {ModuleType::Type::TYPE_ON_OFF}}, // general purpose
		{{CC::SENSOR_BINARY,     0x02}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // smoke
		{{CC::SENSOR_BINARY,     0x03}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // CO
		{{CC::SENSOR_BINARY,     0x04}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // CO2
		{{CC::SENSOR_BINARY,     0x05}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // heat
		{{CC::SENSOR_BINARY,     0x06}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // water
		{{CC::SENSOR_BINARY,     0x07}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // freeze
		{{CC::SENSOR_BINARY,     0x08}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // tamper
		{{CC::SENSOR_BINARY,     0x09}, {ModuleType::Type::TYPE_ON_OFF}}, // auxiliary
		{{CC::SENSOR_BINARY,     0x0A}, {ModuleType::Type::TYPE_OPEN_CLOSE}}, // door/window
		{{CC::SENSOR_BINARY,     0x0B}, {ModuleType::Type::TYPE_ON_OFF}}, // tilt
		{{CC::SENSOR_BINARY,     0x0C}, {ModuleType::Type::TYPE_MOTION}}, // motion
		{{CC::SENSOR_BINARY,     0x0D}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // glass break
		{{CC::ALARM,             0x01}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // smoke
		{{CC::ALARM,             0x02}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // CO
		{{CC::ALARM,             0x03}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // CO2
		{{CC::ALARM,             0x04}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // heat
		{{CC::ALARM,             0x05}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // water
		{{CC::ALARM,             0x07}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // tampering
		{{CC::ALARM,             0x08}, {ModuleType::Type::TYPE_ON_OFF}}, // AC connected/disconnected
		{{CC::ALARM,             0x09}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // system failure
		{{CC::ALARM,             0x0A}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // emergency
		{{CC::ALARM,             0x0B}, {ModuleType::Type::TYPE_SECURITY_ALERT}}, // alarm clock
		{{CC::SENSOR_MULTILEVEL, 0x01}, {ModuleType::Type::TYPE_TEMPERATURE}},
		{{CC::SENSOR_MULTILEVEL, 0x03}, {ModuleType::Type::TYPE_LUMINANCE}},
		{{CC::SENSOR_MULTILEVEL, 0x04}, {ModuleType::Type::TYPE_POWER}},
		{{CC::SENSOR_MULTILEVEL, 0x05}, {ModuleType::Type::TYPE_HUMIDITY}},
		{{CC::SENSOR_MULTILEVEL, 0x0F}, {ModuleType::Type::TYPE_VOLTAGE}},
		{{CC::SENSOR_MULTILEVEL, 0x10}, {ModuleType::Type::TYPE_CURRENT}},
		{{CC::SENSOR_MULTILEVEL, 0x17}, {ModuleType::Type::TYPE_TEMPERATURE}}, // water
		{{CC::SENSOR_MULTILEVEL, 0x18}, {ModuleType::Type::TYPE_TEMPERATURE}}, // soil
		{{CC::SENSOR_MULTILEVEL, 0x1B}, {ModuleType::Type::TYPE_ULTRAVIOLET}},
		{{CC::SENSOR_MULTILEVEL, 0x1E}, {ModuleType::Type::TYPE_NOISE}},
		{{CC::SENSOR_MULTILEVEL, 0x29}, {ModuleType::Type::TYPE_HUMIDITY}}, // soil
		{{CC::SENSOR_MULTILEVEL, 0x40}, {ModuleType::Type::TYPE_TEMPERATURE,
							{ModuleType::Attribute::ATTR_OUTER}}},
		{{CC::BATTERY,           0x00}, {ModuleType::Type::TYPE_BATTERY}},
	};

	m_typesOrder = {
		{{CC::BASIC,             0x00},  0},
		{{CC::BATTERY,           0x00},  1},
		{{CC::SWITCH_BINARY,     0x00},  2},
		{{CC::SENSOR_MULTILEVEL, 0x01},  3},
		{{CC::SENSOR_MULTILEVEL, 0x03},  4},
		{{CC::SENSOR_MULTILEVEL, 0x04},  5},
		{{CC::SENSOR_MULTILEVEL, 0x05},  6},
		{{CC::SENSOR_MULTILEVEL, 0x0F},  7},
		{{CC::SENSOR_MULTILEVEL, 0x10},  8},
		{{CC::SENSOR_MULTILEVEL, 0x17},  9},
		{{CC::SENSOR_MULTILEVEL, 0x18}, 10},
		{{CC::SENSOR_MULTILEVEL, 0x1B}, 11},
		{{CC::SENSOR_MULTILEVEL, 0x1E}, 12},
		{{CC::SENSOR_MULTILEVEL, 0x29}, 13},
		{{CC::SENSOR_MULTILEVEL, 0x40}, 14},
		{{CC::SENSOR_BINARY,     0x00}, 15},
		{{CC::SENSOR_BINARY,     0x01}, 16},
		{{CC::SENSOR_BINARY,     0x02}, 17},
		{{CC::SENSOR_BINARY,     0x03}, 18},
		{{CC::SENSOR_BINARY,     0x04}, 19},
		{{CC::SENSOR_BINARY,     0x05}, 20},
		{{CC::SENSOR_BINARY,     0x06}, 21},
		{{CC::SENSOR_BINARY,     0x07}, 22},
		{{CC::SENSOR_BINARY,     0x08}, 23},
		{{CC::SENSOR_BINARY,     0x09}, 24},
		{{CC::SENSOR_BINARY,     0x0A}, 25},
		{{CC::SENSOR_BINARY,     0x0B}, 26},
		{{CC::SENSOR_BINARY,     0x0C}, 27},
		{{CC::SENSOR_BINARY,     0x0D}, 28},
		{{CC::ALARM,             0x01}, 29},
		{{CC::ALARM,             0x02}, 30},
		{{CC::ALARM,             0x03}, 31},
		{{CC::ALARM,             0x04}, 32},
		{{CC::ALARM,             0x05}, 33},
		{{CC::ALARM,             0x07}, 34},
		{{CC::ALARM,             0x08}, 35},
		{{CC::ALARM,             0x09}, 36},
		{{CC::ALARM,             0x0A}, 37},
		{{CC::ALARM,             0x0B}, 38},
	};
}

void GenericZWaveMapperRegistry::loadTypesMapping(const string &file)
{
	logger().information("loading types-mapping from: " + file);
	FileInputStream in(file);

	loadTypesMapping(in);
}

void GenericZWaveMapperRegistry::loadTypesMapping(istream &in)
{
	ZWaveTypeMappingParser parser;

	map<pair<uint8_t, uint8_t>, ModuleType> typesMapping;
	map<pair<uint8_t, uint8_t>, unsigned int> typesOrder;
	unsigned int order = 0;

	for (const auto &map : parser.parse(in)) {
		const auto zwave = map.first;
		const auto beeeon = map.second;

		auto mResult = typesMapping.emplace(zwave, beeeon);
		if (!mResult.second) {
			throw ExistsException(
				"duplicate Z-Wave type " + to_string(zwave.first)
				+ ":" + to_string(zwave.second));
		}

		auto oResult = typesOrder.emplace(zwave, order++);
		poco_assert(oResult.second);

		if (logger().debug()) {
			logger().debug(
				"mapping " + to_string(zwave.first) + ":" + to_string(zwave.second)
				+ " to " + beeeon.type().toString(),
				__FILE__, __LINE__);
		}
	}

	m_typesMapping = typesMapping;
	m_typesOrder = typesOrder;
}

ZWaveMapperRegistry::Mapper::Ptr GenericZWaveMapperRegistry::resolve(
		const ZWaveNode &node)
{
	if (!node.queried())
		return nullptr;

	logger().information(
		"resolving node " + node.toString()
		+ " (" + to_string(node.commandClasses().size()) + " command classes)",
		__FILE__, __LINE__);

	GenericMapper::Ptr mapper = new GenericMapper(node.id(), node.product());

	map<unsigned int, ZWaveNode::CommandClass> ordered;

	for (const auto &cc : node.commandClasses()) {
		auto it = m_typesOrder.find({cc.id(), cc.index()});
		if (it == m_typesOrder.end()) {
			if (logger().debug()) {
				logger().debug(
					"no module mapping of " + cc.toString()
					+ " for " + node.toString(),
					__FILE__, __LINE__);
			}

			continue;
		}

		ordered.emplace(it->second, cc);
	}

	for (const auto &cc : ordered) {
		auto it = m_typesMapping.find({cc.second.id(), cc.second.index()});
		if (it == m_typesMapping.end()) {
			logger().warning(
				"types order seems to be incompatible with types mapping for "
				+ cc.second.toString(),
				__FILE__, __LINE__);
		}

		logger().information(
			"module mapping " + cc.second.toString()
			+ " as " + it->second.type().toString()
			+ " for " + node.toString(),
			__FILE__, __LINE__);

		mapper->mapType(cc.second, it->second);
	}

	return mapper;
}
