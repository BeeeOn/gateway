#ifndef BEEEON_MODULE_TYPE_H
#define BEEEON_MODULE_TYPE_H

#include <set>

#include "util/Enum.h"

namespace BeeeOn {

/**
 * Representation of value type that device can send. Each value
 * consists of two parts: Type and Attribute.
 *
 * The attribute is optional. The type is high level data type
 * and the attribute is additional information, for example
 * location (inner, outer).
 */
class ModuleType {
public:
	struct AttributeEnum {
		enum Raw {
			TYPE_INNER,
			TYPE_OUTER,
		};

		static EnumHelper<Raw>::ValueMap &valueMap();
	};

	struct TypeEnum {
		enum Raw {
			TYPE_BATTERY,
			TYPE_CO2,
			TYPE_HUMIDITY,
			TYPE_MOTION,
			TYPE_NOISE,
			TYPE_ON_OFF,
			TYPE_PRESSURE,
			TYPE_TEMPERATURE,
		};

		static EnumHelper<Raw>::ValueMap &valueMap();
	};

	typedef Enum<TypeEnum> Type;
	typedef Enum<AttributeEnum> Attribute;

	ModuleType(const Type &type, const std::set<Attribute> &attributes);

	void setType(const Type &type);
	Type type() const;

	void setAttributes(const std::set<Attribute> &attributes);
	std::set<Attribute> attributes() const;

private:
	Type m_type;
	std::set<Attribute> m_attributes;
};

}

#endif
