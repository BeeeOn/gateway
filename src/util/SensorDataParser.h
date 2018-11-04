#pragma once

#include <string>

#include <Poco/SharedPtr.h>

#include "model/SensorData.h"

namespace BeeeOn {

class SensorDataParser {
public:
	typedef Poco::SharedPtr<SensorDataParser> Ptr;

	virtual ~SensorDataParser();

	/**
	 * Convert string representation of SensorData to SensorData object.
	 */
	virtual SensorData parse(const std::string &data) const = 0;
};

}
