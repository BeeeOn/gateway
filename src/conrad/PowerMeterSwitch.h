#pragma once

#include <Poco/SharedPtr.h>

#include "model/SensorData.h"
#include "conrad/ConradDevice.h"

namespace BeeeOn {

/**
 * @brief The class represents a standalone device Conrad Power meter switch.
 * It allows to communicate with the device via Conrad interface.
 */
class PowerMeterSwitch : public ConradDevice {
public:
	typedef Poco::SharedPtr<PowerMeterSwitch> Ptr;

	static const std::string PRODUCT_NAME;

	PowerMeterSwitch(const DeviceID& id, const RefreshTime &refresh);
	~PowerMeterSwitch();

	/**
	 * @brief Parses the message from Conrad interface and creates
	 * from it SensorData.
	 */
	SensorData parseMessage(const Poco::JSON::Object::Ptr message) override;
};

}
