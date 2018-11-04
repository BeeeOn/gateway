#pragma once

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
