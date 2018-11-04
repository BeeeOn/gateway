#pragma once

#include <Poco/SharedPtr.h>

#include "bluetooth/BeeWiDevice.h"

namespace BeeeOn {

/**
 * @brief The class represents BeeWi temperature and humidity sensor. It allows
 * to parse recieved data from the device. Its modules are temperature, humidity
 * and battery level.
 */
class BeeWiSmartClim : public BeeWiDevice {
public:
	typedef Poco::SharedPtr<BeeWiSmartClim> Ptr;

	static const std::string NAME;

public:
	BeeWiSmartClim(const MACAddress& address, const Poco::Timespan& timeout);
	~BeeWiSmartClim();

	/**
	 * <pre>
	 * | ID (1 B) | 1 B | temperature (2 B) | 1 B | humidity (1 B) | 4 B | battery (1 B) |
	 * </pre>
	 */
	SensorData parseAdvertisingData(
		const std::vector<unsigned char>& data) const override;

	static bool match(const std::string& modelID);
};

}
