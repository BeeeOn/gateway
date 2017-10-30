#pragma once

#include "model/SensorData.h"
#include "util/SensorDataParser.h"

namespace BeeeOn {

/**
 * @class JSONSensorDataParser
 * @brief Provides a method to parse SensorData from a string containing JSONObject to a SensorData object
 *
 * Example string:
 *
 *		{"device_id":"0x499602d2","timestamp":150000000000,"data":[{"module_id":5,"value":4.2},{"module_id":4,"value":0.5}]}
 *
 * would be parsed to a SensorData object with relevant values. It is assumed that the "timestamp" contains
 * epochMicroseconds.
 * When a "value" is null, it is parsed to NaN.
 */
class JSONSensorDataParser : SensorDataParser {
public:
	SensorData parse(const std::string &data) const override;
};

}

