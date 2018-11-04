#pragma once

#include <string>

#include <Poco/SharedPtr.h>

namespace BeeeOn {

class SensorData;

class SensorDataFormatter {
public:
	typedef Poco::SharedPtr<SensorDataFormatter> Ptr;

	SensorDataFormatter();
	virtual ~SensorDataFormatter();

	/**
	 * Convert data from struct SensorData to some formatted text
	 */
	virtual std::string format(const SensorData &data) = 0;
};

}
