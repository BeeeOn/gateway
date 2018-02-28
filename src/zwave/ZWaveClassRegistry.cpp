#include <Poco/SingletonHolder.h>

#include "zwave/ZWaveClassRegistry.h"
#include "zwave/ZWaveUtil.h"

using namespace BeeeOn;
using namespace Poco;

static const ZWaveCommandClassMap COMMON_TYPES = {
	{{37, 0x00}, {ModuleType::Type::TYPE_ON_OFF, {ModuleType::Attribute::ATTR_CONTROLLABLE}}},
	{{49, 0x01}, {ModuleType::Type::TYPE_TEMPERATURE}},
	{{49, 0x03}, {ModuleType::Type::TYPE_LUMINANCE}},
	{{49, 0x05}, {ModuleType::Type::TYPE_HUMIDITY}},
	{{49, 0x1b}, {ModuleType::Type::TYPE_ULTRAVIOLET}},
	{{128, 0x00}, {ModuleType::Type::TYPE_BATTERY}},
};

ZWaveClassRegistry::~ZWaveClassRegistry()
{
}

ZWaveCommonClassRegistry::ZWaveCommonClassRegistry()
{
}

ZWaveCommonClassRegistry &ZWaveCommonClassRegistry::instance()
{
	static SingletonHolder<ZWaveCommonClassRegistry> sh;
	return *sh.get();
}

ModuleType ZWaveCommonClassRegistry::find(
	uint8_t commandClass, uint8_t index)
{
	auto it = COMMON_TYPES.find({commandClass, index});
	if (it == COMMON_TYPES.end())
		throw NotFoundException("no type for "
			+ ZWaveUtil::commandClass(commandClass, index));

	return it->second;
}

bool ZWaveCommonClassRegistry::contains(
	uint8_t commandClass, uint8_t index)
{
	return COMMON_TYPES.find({commandClass, index})
		!= COMMON_TYPES.end();
}

ZWaveGenericClassRegistry::ZWaveGenericClassRegistry(
		const ZWaveCommandClassMap &map):
	m_map(map)
{
}

ModuleType ZWaveGenericClassRegistry::find(
	uint8_t commandClass, uint8_t index)
{
	auto it = m_map.find({commandClass, index});
	if (it != m_map.end()) {
		return it->second;
	}

	throw NotFoundException("no type for "
		+ ZWaveUtil::commandClass(commandClass, index));
}

bool ZWaveGenericClassRegistry::contains(
	uint8_t commandClass, uint8_t index)
{
	return m_map.find({commandClass, index}) != m_map.end();
}

ZWaveProductClassRegistry::ZWaveProductClassRegistry(
		const ZWaveCommandClassMap &map):
	m_impl(map)
{
}

ModuleType ZWaveProductClassRegistry::find(
	uint8_t commandClass, uint8_t index)
{
	if (m_impl.contains(commandClass, index))
		return m_impl.find(commandClass, index);

	return ZWaveCommonClassRegistry::instance()
		.find(commandClass, index);
}

bool ZWaveProductClassRegistry::contains(
	uint8_t commandClass, uint8_t index)
{
	if (m_impl.contains(commandClass, index))
		return true;

	return ZWaveCommonClassRegistry::instance()
		.contains(commandClass, index);
}
