#ifndef BEEEON_MODULE_ID_H
#define BEEEON_MODULE_ID_H

#include <cstdint>
#include <string>

namespace BeeeOn {

/*
 * Represents the identification of a sensor module, type of a measured value
 * (temperature, pressure, motion, ...).
 */
class ModuleID {
public:
	/*
	 * Construct ModuleID with default ModuleID.
	 */
	ModuleID();

	ModuleID(const uint16_t &moduleID);

	uint16_t value() const
	{
		return m_moduleID;
	}

	static ModuleID parse(const std::string &s);

	std::string toString() const;

	bool operator !=(const ModuleID &id) const
	{
		return m_moduleID != id.m_moduleID;
	}

	bool operator ==(const ModuleID &id) const
	{
		return m_moduleID == id.m_moduleID;
	}

	bool operator <(const ModuleID &id) const
	{
		return m_moduleID < id.m_moduleID;
	}

	bool operator >(const ModuleID &id) const
	{
		return m_moduleID > id.m_moduleID;
	}

	bool operator <=(const ModuleID &id) const
	{
		return m_moduleID <= id.m_moduleID;
	}

	bool operator >=(const ModuleID &id) const
	{
		return m_moduleID >= id.m_moduleID;
	}

	operator unsigned short() const
	{
		return m_moduleID;
	}

	operator uint16_t()
	{
		return m_moduleID;
	}

private:
	uint16_t m_moduleID;
};

}

#endif
