#pragma once

#include <map>
#include <string>

#include <Poco/SharedPtr.h>
#include <Poco/Instantiator.h>

#include "util/Loggable.h"
#include "zwave/ZWaveMapperRegistry.h"

namespace BeeeOn {

/**
 * @brief SpecificZWaveMapperRegistry implements the method resolve()
 * generically. The subclass of SpecificZWaveMapperRegistry should
 * register a special instantiator creating the appropriate Mapper
 * implementation based on the vendor and product IDs of Z-Wave nodes.
 */
class SpecificZWaveMapperRegistry : public ZWaveMapperRegistry, protected Loggable {
public:
	SpecificZWaveMapperRegistry();

	Mapper::Ptr resolve(const ZWaveNode &node) override;

	/**
	 * @brief Set the spec mapping where the map key is a string
	 * in form: <code>VENDOR:PRODUCT</code> and the value is the
	 * name of MapperInstantiator to be used.
	 */
	void setSpecMap(const std::map<std::string, std::string> &specMap);

protected:
	/**
	 * @brief Specification of a Z-Wave node to match.
	 */
	struct Spec {
		const uint16_t vendor;
		const uint16_t product;

		bool operator <(const Spec &other) const;
		std::string toString() const;
		static Spec parse(const std::string &input);
	};

	/**
	 * @brief Instantiator of specific Mapper implementations.
	 */
	class MapperInstantiator {
	public:
		typedef Poco::SharedPtr<MapperInstantiator> Ptr;

		virtual ~MapperInstantiator();

		virtual Mapper::Ptr create(const ZWaveNode &node) = 0;
	};

	/**
	 * @brief Template implementation of MapperInstantiator creating
	 * MapperType instances having constructor specified as:
	 * <code>MapperType(const ZWaveNode::Identity &, const std::string &)</code>.
	 */
	template <typename MapperType>
	class SimpleMapperInstantiator : public MapperInstantiator {
	public:
		Mapper::Ptr create(const ZWaveNode &node) override
		{
			return new MapperType(node.id(), node.product());
		}
	};

	/**
	 * @brief The subclass would call this method for each instantiator
	 * type it offers. The name of instantiator is referred from the
	 * specMap property.
	 *
	 * @see setSpecMap()
	 */
	void registerInstantiator(
		const std::string &name,
		MapperInstantiator::Ptr instantiator);

private:
	typedef std::map<std::string, MapperInstantiator::Ptr> InstantiatorsMap;
	InstantiatorsMap m_instantiators;
	std::map<Spec, InstantiatorsMap::iterator> m_specMap;
};

}
