#ifndef BEEEON_NULL_SENSOR_DATA_FORMATTER_H
#define BEEEON_NULL_SENSOR_DATA_FORMATTER_H

#include <string>

#include "util/SensorDataFormatter.h"

namespace BeeeOn {

class SensorData;

/**
 * NullSensorDataFormatter can be and is used as a default
 * instance of a SensorDataFormatter (instead of NULL pointer).
 */
class NullSensorDataFormatter : public SensorDataFormatter {
public:
	NullSensorDataFormatter();
	~NullSensorDataFormatter();

	static SensorDataFormatter &instance();

	std::string format(const SensorData &data) override;
};

}

#endif // BEEEON_NULL_SENSOR_DATA_FORMATTER_H
