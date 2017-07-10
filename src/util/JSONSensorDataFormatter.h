#ifndef BEEEON_JSON_SENSOR_DATA_FORMATTER_H
#define BEEEON_JSON_SENSOR_DATA_FORMATTER_H

#include <string>

#include "util/SensorDataFormatter.h"

namespace BeeeOn {

class SensorData;

class JSONSensorDataFormatter : public SensorDataFormatter {
public:
	JSONSensorDataFormatter();

	/**
	 * Convert data from struct SensorData to JSON format
	 */
	std::string format(const SensorData &data) override;
};

}

#endif // BEEEON_JSON_SENSOR_DATA_FORMATTER_H
