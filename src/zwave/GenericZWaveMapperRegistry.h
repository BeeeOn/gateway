#pragma once

#include <iosfwd>
#include <map>

#include "model/ModuleID.h"
#include "model/ModuleType.h"
#include "util/Loggable.h"
#include "zwave/ZWaveMapperRegistry.h"

namespace BeeeOn {

/**
 * @brief Certain Z-Wave nodes can be mapped to the BeeeOn system generically.
 * This allows to support any node type without knowing it in advance. The
 * GenericZWaveMapperRegistry implements this approach.
 *
 * Devices resolved by the GenericZWaveMapperRegistry would have mangled device
 * ID with 0xff in the top bits of the ident part.
 *
 * Reported devices resolved by the GenericZWaveMapperRegistry would have a
 * product name with appended string " (generic)". Thus, such devices can be
 * distinguished easily from specific implementations that might have different
 * module types.
 */
class GenericZWaveMapperRegistry :
	public ZWaveMapperRegistry,
	Loggable {
public:
	class GenericMapper : public ZWaveMapperRegistry::Mapper {
	public:
		typedef Poco::SharedPtr<GenericMapper> Ptr;

		/**
		 * @brief Mangle bits to be injected into device ID of Z-Wave
		 * devices that are using the GenericMapper.
		 */
		static uint8_t ID_MANGLE_BITS;

		GenericMapper(const ZWaveNode::Identity &id, const std::string &product);

		/**
		 * @returns device ID for the given Z-Wave identity mangled by
		 * the ID_MANGLE_BITS (0xff).
		 */
		DeviceID buildID() const override;

		/**
		 * @returns product name of the ZWaveNode with appended " (generic)"
		 * string to be easily distinguishable.
		 */
		std::string product() const override;

		/**
		 * @returns resolved module types
		 */
		std::list<ModuleType> types() const override;
		SensorValue convert(const ZWaveNode::Value &value) const override;

		/**
		 * @brief Record the given association between the given Z-Wave
		 * command class and BeeeOn ModuleType. When calling this method
		 * mutiple times, the order corresponds to an increasing ModuleID.
		 */
		void mapType(
			const ZWaveNode::CommandClass &cc,
			const ModuleType &type);

	protected:
		/**
		 * @brief Throw an exception when the given value cannot be converted
		 * into the BeeeOn representation.
		 */
		void cannotConvert(const ZWaveNode::Value &value) const;

	private:
		std::map<ZWaveNode::CommandClass, ModuleID> m_mapping;
		std::map<ModuleID, ModuleType> m_modules;
	};

	GenericZWaveMapperRegistry();

	/**
	 * @brief Load XML file with the types mapping between Z-Wave and BeeeOn.
	 */
	void loadTypesMapping(const std::string &file);
	void loadTypesMapping(std::istream &in);

	/**
	 * @breif Map the given ZWaveNode instance on-fly to the BeeeOn system
	 * by using the GenericMapper.
	 */
	Mapper::Ptr resolve(const ZWaveNode &node) override;

private:
	/**
	 * The m_typesMapping maps Z-Wave command classes to BeeeOn types.
	 */
	std::map<std::pair<uint8_t, uint8_t>, ModuleType> m_typesMapping;

	/**
	 * The m_typesOrder maintains backwards compatibility of the m_typesMapping and GenericMapper.
	 * If a new data type is added, it MUST be appended to the end of the m_typesOrder and its
	 * value must be incremented by 1 from the last one.
	 *
	 * The values as discovered from Z-Wave nodes must be always in the same order to
	 * have a stable mapping to BeeeOn modules of a device. When all values are reported
	 * for a Z-Wave device, they are sorted according to the m_typesOrder.
	 */
	std::map<std::pair<uint8_t, uint8_t>, unsigned int> m_typesOrder;
};

}
