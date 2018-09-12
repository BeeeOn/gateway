#pragma once

#include <string>

#include "util/SensorDataFormatter.h"

namespace BeeeOn {

class SensorData;

class CSVSensorDataFormatter : public SensorDataFormatter {
public:
	CSVSensorDataFormatter();

	/**
	 * Convert data from struct SensorData to csv format
	 * @return type;timestamp;deviceID;moduleID;value;
	 *
	 * Example output:
	 * <pre>sensor;1488879656;0x499602d2;5;4.200000;</pre>
	 */
	std::string format(const SensorData &data) override;

	/**
	 * Optional custom separator
	 */
	void setSeparator(const std::string &separator)
	{
		m_separator = separator;
	}

	/**
	 * @brief separator
	 * @return actual separator for csv
	 */
	const std::string separator() const
	{
		return m_separator;
	}

private:
	std::string m_separator;
};

}
