#ifndef BEEEON_SENSOR_DATA_H
#define BEEEON_SENSOR_DATA_H

#include <vector>

#include "model/DeviceID.h"
#include "model/ModuleID.h"
#include "model/SensorValue.h"
#include "util/IncompleteTimestamp.h"

namespace BeeeOn {

/*
 * Representation of data coming from a sensor. Every SensorData comes
 * from a single device identified by DeviceID. The SensorData holds
 * a list of measured values.Each of measured values is defined by
 * SensorValue instance.
 */
class SensorData {
public:
	void setDeviceID(const DeviceID &deviceID)
	{
		m_deviceID = deviceID;
	}

	const DeviceID deviceID() const
	{
		return m_deviceID;
	}

	void setTimestamp(const IncompleteTimestamp &timestamp)
	{
		m_timestamp = timestamp;
	}

	const IncompleteTimestamp timestamp() const
	{
		return m_timestamp;
	}

	void insertValue(const SensorValue &value)
	{
		m_values.push_back(value);
	}

	std::vector<SensorValue>::iterator begin()
	{
		return m_values.begin();
	}

	std::vector<SensorValue>::iterator end()
	{
		return m_values.end();
	}

	std::vector<SensorValue>::const_iterator begin() const
	{
		return m_values.begin();
	}

	std::vector<SensorValue>::const_iterator end() const
	{
		return m_values.end();
	}

private:
	DeviceID m_deviceID;
	IncompleteTimestamp m_timestamp;
	std::vector<SensorValue> m_values;
};

}

#endif
