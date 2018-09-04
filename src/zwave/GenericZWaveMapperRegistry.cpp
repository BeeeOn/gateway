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

ZWaveNode::Value GenericZWaveMapperRegistry::GenericMapper::convert(
		const ModuleID &id,
		double value) const
{
	Nullable<ModuleType> type = findType(id);
	if (type.isNull()) {
		throw NotFoundException(
			"no such controllable module " + id.toString()
			+ " for " + buildID().toString());
	}

	auto it = m_mapping.begin();
	for (; it != m_mapping.end(); ++it) {
		if (it->second == id)
			break;
	}

	if (it == m_mapping.end()) {
		throw NotFoundException(
			"module " + id.toString()
			+ " is not mapped for " + buildID().toString());
	}

	auto &cc = it->first;

	switch (type.value().type()) {
	case ModuleType::Type::TYPE_ON_OFF:
		return ZWaveNode::Value(identity(), cc, to_string(value != 0));

	default:
		throw NotImplementedException(
			"type " + type.value().type().toString()
			+ " is unsupported for Z-Wave as controllable");
	}
}

GenericZWaveMapperRegistry::GenericZWaveMapperRegistry()
{
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
