#ifndef BEEEON_CSV_SENSOR_DATA_FORMATTER_H
#define BEEEON_CSV_SENSOR_DATA_FORMATTER_H

#include <string>

#include "SensorDataFormatter.h"

namespace BeeeOn {

class SensorData;

class CSVSensorDataFormatter : public SensorDataFormatter {
public:
	CSVSensorDataFormatter();

	/**
	 * Convert data from struct SensorData to csv format
	 * @return type;timestamp;deviceID;moduleID;value;
	 * @example sensor;1488879656;0x499602d2;5;4.200000;
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

#endif // BEEEON_CSV_SENSOR_DATA_FORMATTER_H
