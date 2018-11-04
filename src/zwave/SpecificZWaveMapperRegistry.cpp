#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>

#include "zwave/SpecificZWaveMapperRegistry.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

bool SpecificZWaveMapperRegistry::Spec::operator <(const Spec &other) const
{
	if (vendor < other.vendor)
		return true;
	if (vendor > other.vendor)
		return false;

	return product < other.product;
}

string SpecificZWaveMapperRegistry::Spec::toString() const
{
	return NumberFormatter::formatHex(vendor, 4)
		+ ":"
		+ NumberFormatter::formatHex(product, 4);
}

SpecificZWaveMapperRegistry::Spec SpecificZWaveMapperRegistry::Spec::parse(
		const string &input)
{
	static const RegularExpression specRegex("^([0-9a-fA-F]{4}):([0-9a-fA-F]{4})$");

	RegularExpression::MatchVec match;
	if (!specRegex.match(input, 0, match))
		throw SyntaxException("given device spec '" + input + "' is invalid");

	const auto vendor = input.substr(match[1].offset, match[1].length);
	const auto product = input.substr(match[2].offset, match[2].length);

	return {
		static_cast<uint16_t>(NumberParser::parseHex(vendor)),
		static_cast<uint16_t>(NumberParser::parseHex(product))
	};
}

SpecificZWaveMapperRegistry::MapperInstantiator::~MapperInstantiator()
{
}

SpecificZWaveMapperRegistry::SpecificZWaveMapperRegistry()
{
}

ZWaveMapperRegistry::Mapper::Ptr SpecificZWaveMapperRegistry::resolve(
		const ZWaveNode &node)
{
	auto it = m_specMap.find({node.vendorId(), node.productId()});
	if (it == m_specMap.end())
		return nullptr;

	logger().information(
		"resolved node " + node.toString() + " as " + it->second->first,
		__FILE__, __LINE__);

	auto instantiator = it->second->second;
	return instantiator->create(node);
}

void SpecificZWaveMapperRegistry::setSpecMap(const map<string, string> &specMap)
{
	map<Spec, InstantiatorsMap::iterator> newSpecMap;

	for (const auto pair : specMap) {
		const auto &spec = Spec::parse(pair.first);
		auto it = m_instantiators.find(pair.second);

		if (it == m_instantiators.end()) {
			throw NotFoundException(
				"no such Mapper instantiator named '" + pair.second + "'");
		}

		auto result = newSpecMap.emplace(spec, it);
		if (!result.second) {
			throw ExistsException(
				"spec '" + spec.toString() + "' already exists");
		}
	}

	m_specMap = newSpecMap;
}

void SpecificZWaveMapperRegistry::registerInstantiator(
		const std::string &name,
		const MapperInstantiator::Ptr instantiator)
{
	const auto result = m_instantiators.emplace(name, instantiator);
	if (!result.second)
		throw ExistsException("Mapper instantiator " + name + " already exists");
}
