#include "model/ModuleType.h"

using namespace BeeeOn;
using namespace std;

EnumHelper<ModuleType::AttributeEnum::Raw>::ValueMap &ModuleType::AttributeEnum::valueMap()
{
	static EnumHelper<ModuleType::AttributeEnum::Raw>::ValueMap valueMap = {
		{ModuleType::AttributeEnum::TYPE_INNER, "inner"},
		{ModuleType::AttributeEnum::TYPE_OUTER, "outer"},
	};

	return valueMap;
}

EnumHelper<ModuleType::TypeEnum::Raw>::ValueMap &ModuleType::TypeEnum::valueMap()
{
	static EnumHelper<ModuleType::TypeEnum::Raw>::ValueMap valueMap = {
		{ModuleType::TypeEnum::TYPE_BATTERY, "battery"},
		{ModuleType::TypeEnum::TYPE_CO2, "co2"},
		{ModuleType::TypeEnum::TYPE_HUMIDITY, "humidity"},
		{ModuleType::TypeEnum::TYPE_MOTION, "motion"},
		{ModuleType::TypeEnum::TYPE_NOISE, "noise"},
		{ModuleType::TypeEnum::TYPE_ON_OFF, "on-off"},
		{ModuleType::TypeEnum::TYPE_PRESSURE, "pressure"},
		{ModuleType::TypeEnum::TYPE_TEMPERATURE, "temperature"},
	};

	return valueMap;
}

ModuleType::ModuleType(const ModuleType::Type &type,
		const set<ModuleType::Attribute> &attributes):
	m_type(type),
	m_attributes(attributes)
{
}

void ModuleType::setType(const ModuleType::Type &type)
{
	m_type = type;
}

ModuleType::Type ModuleType::type() const
{
	return m_type;
}

void ModuleType::setAttributes(const set<ModuleType::Attribute> &attributes)
{
	m_attributes = attributes;
}

set<ModuleType::Attribute> ModuleType::attributes() const
{
	return m_attributes;
}
