#include <Poco/FileStream.h>
#include <Poco/Logger.h>

#include "iqrf/DPAMappedProtocol.h"

using namespace std;
using namespace BeeeOn;
using namespace Poco;

DPAMappedProtocol::DPAMappedProtocol(
		const string &mappingGroup,
		const string &techNode):
	m_mappingGroup(mappingGroup),
	m_techNode(techNode)
{
}

void DPAMappedProtocol::loadTypesMapping(const string &file)
{
	logger().information(
		"loading types-mapping from: " + file,
		__FILE__, __LINE__);

	FileInputStream in(file);

	loadTypesMapping(in);
}

void DPAMappedProtocol::loadTypesMapping(istream &in)
{
	IQRFTypeMappingParser parser(m_mappingGroup, m_techNode);

	map<uint8_t, ModuleType> moduleTypes;
	map<uint8_t, IQRFType> iqrfTypes;

	for (const auto &pair : parser.parse(in)) {
		const auto &iqrf = pair.first;
		const auto &beeeon = pair.second;

		auto itModuleType = moduleTypes.emplace(iqrf.id, beeeon);
		if (!itModuleType.second) {
			throw ExistsException(
				"duplicate module type type with id " + to_string(iqrf.id));
		}

		auto itIQRFType = iqrfTypes.emplace(iqrf.id, iqrf);
		if (!itIQRFType.second) {
			throw ExistsException(
				"duplicate IQRF type type with id " + to_string(iqrf.id));
		}

		if (logger().debug()) {
			logger().debug(
				"mapping " + iqrf.toString()
				+ " to " + beeeon.type().toString(),
				__FILE__, __LINE__);
		}
	}

	m_moduleTypes = moduleTypes;
	m_iqrfTypes = iqrfTypes;
}

SensorValue DPAMappedProtocol::extractSensorValue(
		const ModuleID &moduleID,
		const IQRFType &type,
		const uint16_t value) const
{
	if (value == type.errorValue)
		return {moduleID};

	if (type.signedFlag)
		return {moduleID, static_cast<int16_t>(value) * type.resolution};

	return {moduleID, value * type.resolution};
}

list<ModuleType> DPAMappedProtocol::extractModules(
		const vector<uint8_t> &message) const
{
	list<ModuleType> modules;

	for (uint8_t byte : message)
		modules.emplace_back(findModuleType(byte));

	modules.emplace_back(ModuleType::Type::TYPE_BATTERY);
	modules.emplace_back(ModuleType::Type::TYPE_RSSI);

	return modules;
}

SensorData DPAMappedProtocol::parseValue(
		const list<ModuleType> &modules,
		const vector<uint8_t> &msg) const
{
	SensorData data;
	uint16_t moduleID = 0;
	size_t i = 0;

	while (i < msg.size() && moduleID < modules.size()) {
		const auto &iqrfType = findIQRFType(msg.at(i));

		uint16_t value;
		if (iqrfType.wide == 1)
			value = msg.at(i + 1);
		else if (iqrfType.wide == 2)
			value = static_cast<uint16_t>(msg.at(i + 2) << 8) | msg.at(i + 1);
		else
			throw InvalidArgumentException("invalid value wide");

		data.insertValue(
			extractSensorValue(moduleID, iqrfType, value));

		i += iqrfType.wide + 1;
		moduleID++;
	}

	return data;
}

ModuleType DPAMappedProtocol::findModuleType(uint8_t id) const
{
	auto it = m_moduleTypes.find(id);
	if (it == m_moduleTypes.end()) {
		throw NotFoundException(
			"unsupported module type with id "
			+ NumberFormatter::formatHex(id, true));
	}

	return it->second;
}

IQRFType DPAMappedProtocol::findIQRFType(uint8_t id) const
{
	auto it = m_iqrfTypes.find(id);
	if (it == m_iqrfTypes.end()) {
		throw NotFoundException(
			"unsupported IQRF type with id "
			+ NumberFormatter::formatHex(id, true));
	}

	return it->second;
}
