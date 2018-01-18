#pragma once

#include <Poco/SharedPtr.h>

#include "model/ModuleType.h"

namespace BeeeOn {

struct ZWaveCommandClassKey {
	uint8_t commandClassID;
	uint8_t index;

	bool operator <(const ZWaveCommandClassKey &id) const
	{
		if (commandClassID < id.commandClassID)
			return true;

		if (commandClassID == id.commandClassID)
			return index < id.index;

		return false;
	}
};

typedef std::map<ZWaveCommandClassKey, ModuleType> ZWaveCommandClassMap;

/**
 * Interface for querying ModuleType based on Z-Wave data types.
 */
class ZWaveClassRegistry {
public:
	typedef Poco::SharedPtr<ZWaveClassRegistry> Ptr;

	virtual ~ZWaveClassRegistry();

	virtual ModuleType find(
		uint8_t commandClass, uint8_t index) = 0;

	virtual bool contains(
		uint8_t commandClass, uint8_t index) = 0;
};

/**
 * It ensures to find and check of ModuleType in map.
 */
class ZWaveGenericClassRegistry : public ZWaveClassRegistry {
public:
	ZWaveGenericClassRegistry(const ZWaveCommandClassMap &map);

	ModuleType find(uint8_t commandClass, uint8_t index) override;
	bool contains(uint8_t commandClass, uint8_t index) override;

private:
	ZWaveCommandClassMap m_map;
};

/**
 * Implementation of common Z-Wave command classes,
 * common indexes and their ModuleType for all devices.
 */
class ZWaveCommonClassRegistry : public ZWaveClassRegistry {
public:
	ZWaveCommonClassRegistry();

	ModuleType find(uint8_t commandClass, uint8_t index) override;
	bool contains(uint8_t commandClass, uint8_t index) override;

	static ZWaveCommonClassRegistry& instance();
};

/**
 * Implementation of specific product of Z-Wave command classes,
 * its indexes and their ModuleType.
 */
class ZWaveProductClassRegistry : public ZWaveClassRegistry {
public:
	ZWaveProductClassRegistry(const ZWaveCommandClassMap &map);

	ModuleType find(uint8_t commandClass, uint8_t index) override;

	bool contains(uint8_t commandClass, uint8_t index) override;

private:
	ZWaveGenericClassRegistry m_impl;
};

}
