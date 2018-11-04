#pragma once

#include <vector>

#include "zwave/ZWaveMapperRegistry.h"

namespace BeeeOn {

/**
 * @brief CompositeZWaveMapperRegistry allows to use multiple different
 * ZWaveMapperRegistry instances by the ZWaveDeviceManager. Thus, it is
 * possible to implement different device recognition strategies.
 *
 * All registered ZWaveMapperRegistry instances are iterated in the order
 * as they have been added. Thus, the last one might be the most generic
 * one.
 */
class CompositeZWaveMapperRegistry : public ZWaveMapperRegistry {
public:
	CompositeZWaveMapperRegistry();

	/**
	 * @brief Try to resolve a Mapper for the given node by
	 * iterating over the registered ZWaveMapperRegistry instances.
	 * When a ZWaveMapperRegistry instance returns a valid Mapper
	 * instance (non-null), it is returned.
	 */
	Mapper::Ptr resolve(const ZWaveNode &node) override;

	/**
	 * @brief Register the given registry to use by resolve().
	 */
	void addRegistry(ZWaveMapperRegistry::Ptr registry);

private:
	std::vector<ZWaveMapperRegistry::Ptr> m_registry;
};

}
