#pragma once

#include <string>

#include "model/SensorData.h"

namespace BeeeOn {

class SensorDataParser {
public:

	/**
	 * Convert string representation of SensorData to SensorData object.
	 */
	virtual SensorData parse(const std::string &data) const = 0;
};

}
