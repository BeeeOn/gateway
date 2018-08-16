#pragma once

#include <list>
#include <string>

#include <Poco/Nullable.h>
#include <Poco/SharedPtr.h>

#include "model/DeviceID.h"
#include "model/ModuleType.h"
#include "model/SensorValue.h"
#include "zwave/ZWaveNode.h"

namespace BeeeOn {

/**
 * @brief ZWaveMapperRegistry is mostly intended to map Z-Wave specific data type
 * hierarchy to BeeeOn ModuleType. Based on the ZWaveNode metadata, it constructs
 * or looks up an appropriate Mapper object that knows how to interpret the
 * ZWaveNode::Value instances to the rest of the BeeeOn system.
 */
class ZWaveMapperRegistry {
public:
	typedef Poco::SharedPtr<ZWaveMapperRegistry> Ptr;

	/**
	 * @brief Map the ZWaveNode-specific data to the BeeeOn-specific ones.
	 * It is assumed that the ZWaveNode instance (or its Value) passed into
	 * the Mapper is the one used by ZWaveMapperRegistry::lookup().
	 */
	class Mapper {
	public:
		typedef Poco::SharedPtr<Mapper> Ptr;

		Mapper(const ZWaveNode::Identity &id, const std::string &product);
		virtual ~Mapper();

		/**
		 * @brief The mapper can sometimes need to mangle a device ID
		 * for a Z-Wave node. This is possible by overriding this method.
		 *
		 * The default implementation builds the ID from the home ID and
		 * node ID.
		 *
		 * @returns ID of the given Z-Wave node identity
		 */
		virtual DeviceID buildID() const;

		/**
		 * @returns fixed product name of the node if needed
		 */
		virtual std::string product() const;

		/**
		 * @returns list of ModuleType instance for a particular Z-Wave node type
		 */
		virtual std::list<ModuleType> types() const = 0;

		/**
		 * @brief Find module type by ID.
		 */
		Poco::Nullable<ModuleType> findType(const ModuleID &id) const;

		/**
		 * @returns SensorValue representation of the ZWaveNode::Value instance
		 */
		virtual SensorValue convert(const ZWaveNode::Value &value) const = 0;

	protected:
		static DeviceID mangleID(const DeviceID &id, uint8_t bits);

	private:
		ZWaveNode::Identity m_identity;
		std::string m_product;
	};

	virtual ~ZWaveMapperRegistry();

	/**
	 * @brief Try to resolve a Mapper implementation suitable for the given Z-Wave node.
	 */
	virtual Mapper::Ptr resolve(const ZWaveNode &node) = 0;
};

}
