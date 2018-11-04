#include "di/Injectable.h"
#include "zwave/CompositeZWaveMapperRegistry.h"

BEEEON_OBJECT_BEGIN(BeeeOn, CompositeZWaveMapperRegistry)
BEEEON_OBJECT_CASTABLE(ZWaveMapperRegistry)
BEEEON_OBJECT_PROPERTY("registry", &CompositeZWaveMapperRegistry::addRegistry)
BEEEON_OBJECT_END(BeeeOn, CompositeZWaveMapperRegistry)

using namespace BeeeOn;

CompositeZWaveMapperRegistry::CompositeZWaveMapperRegistry()
{
}

ZWaveMapperRegistry::Mapper::Ptr CompositeZWaveMapperRegistry::resolve(
		const ZWaveNode &node)
{
	for (auto registry : m_registry) {
		Mapper::Ptr mapper = registry->resolve(node);

		if (!mapper.isNull())
			return mapper;
	}

	return nullptr;
}

void CompositeZWaveMapperRegistry::addRegistry(
		ZWaveMapperRegistry::Ptr registry)
{
	m_registry.emplace_back(registry);
}
