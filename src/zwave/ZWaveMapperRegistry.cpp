#include "zwave/ZWaveMapperRegistry.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

ZWaveMapperRegistry::Mapper::Mapper(
		const ZWaveNode::Identity &id,
		const string &product):
	m_identity(id),
	m_product(product)
{
}

ZWaveMapperRegistry::Mapper::~Mapper()
{
}

DeviceID ZWaveMapperRegistry::Mapper::buildID() const
{
	uint64_t result = 0;
	const uint64_t home64 = m_identity.home;

	result |= home64 << 8;
	result |= m_identity.node;

	return DeviceID(DevicePrefix::PREFIX_ZWAVE, result);
}

string ZWaveMapperRegistry::Mapper::product() const
{
	return m_product;
}

Nullable<ModuleType> ZWaveMapperRegistry::Mapper::findType(const ModuleID &id) const
{
	unsigned int i = 0;

	for (const auto &type : types()) {
		if (i == id)
			return type;

		i += 1;
	}

	Nullable<ModuleType> null;
	return null;
}

static uint64_t ZWAVE_IDENT_MASK = 0x0ffffffffff;
static uint64_t ZWAVE_MANGLE_SHIFT = 40;

DeviceID ZWaveMapperRegistry::Mapper::mangleID(const DeviceID &id, uint8_t bits)
{
	const uint64_t ident = id.ident() & ZWAVE_IDENT_MASK;
	const uint64_t mangleBits = static_cast<uint64_t>(bits) << ZWAVE_MANGLE_SHIFT;

	return {id.prefix(), mangleBits | ident};
}

ZWaveNode::Value ZWaveMapperRegistry::Mapper::convert(
		const ModuleID &,
		double) const
{
	throw NotImplementedException("reverse conversion not supported");
}

ZWaveNode::Identity ZWaveMapperRegistry::Mapper::identity() const
{
	return m_identity;
}

ZWaveMapperRegistry::~ZWaveMapperRegistry()
{
}
