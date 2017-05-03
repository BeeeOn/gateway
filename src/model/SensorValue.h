#ifndef BEEEON_SENSOR_VALUE_H
#define BEEEON_SENSOR_VALUE_H

#include "model/ModuleID.h"

namespace BeeeOn {

/*
 * Representation of a single value coming from a sensor. SensorValue
 * contains module identification, value and valid flag.
 * The class allows to represent:
 *  - invalid value coming from a module (valid flag is false)
 *  - valid value coming from a module.
 */
class SensorValue {
public:
	/*
	 * Construct a default invalid sensor value. The isValid() method
	 * would return false.
	 */
	SensorValue();
	SensorValue(const ModuleID &moduleID);

	/*
	 * Construct a valid sensor value. The isValid() method
	 * would return true.
	 */
	SensorValue(const ModuleID &moduleID, const double &value);

	void setModuleID(const ModuleID &moduleID)
	{
		m_moduleID = moduleID;
	}

	const ModuleID moduleID() const
	{
		return m_moduleID;
	}

	void setValue(const double &value)
	{
		m_value = value;
	}

	double value() const
	{
		return m_value;
	}

	void setValid(const bool valid)
	{
		m_valid = valid;
	}

	bool isValid() const
	{
		return m_valid;
	}

	/*
	 * If the SensorValue is invalid it doesn't check the value.
	 */
	bool operator !=(const SensorValue &value) const
	{
		return !(*this == value);
	}

	/*
	 * If the SensorValue is invalid it doesn't check the value.
	 */
	bool operator ==(const SensorValue &value) const
	{
		if (m_moduleID != value.m_moduleID)
			return false;

		if (m_valid != value.m_valid)
			return false;

		if (m_valid)
			return m_value == value.m_value;

		return true;
	}

private:
	ModuleID m_moduleID;
	double m_value;
	bool m_valid;
};

}

#endif
