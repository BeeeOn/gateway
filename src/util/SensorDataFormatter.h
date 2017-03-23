#ifndef BEEEON_SENSOR_DATA_FORMATTER_H
#define BEEEON_SENSOR_DATA_FORMATTER_H

#include <string>

namespace BeeeOn {

class SensorData;

class SensorDataFormatter {
public:
	SensorDataFormatter();
	virtual ~SensorDataFormatter();

	/**
	 * Convert data from struct SensorData to some formatted text
	 */
	virtual std::string format(const SensorData &data) = 0;
};

}

#endif // BEEEON_SENSOR_DATA_FORMATTER_H
