#pragma once

#include <list>

#include <Poco/SharedPtr.h>
#include <Poco/Timestamp.h>

#include "model/ModuleType.h"
#include "model/SensorData.h"
#include "net/MqttMessage.h"
#include "sonoff/SonoffDevice.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief The class represents Sonoff SC device. The class
 * allows to process the messages from the Sonoff SC with
 * the custom firmware (https://github.com/xbedna62/SonoffSC_firmware.git).
 * Its modules are temperature, humidity, noise, dust and light.
 */
class SonoffSC : public SonoffDevice {
public:
	typedef Poco::SharedPtr<SonoffSC> Ptr;

	static const std::string PRODUCT_NAME;

	SonoffSC(const DeviceID& id, const RefreshTime &refresh);
	~SonoffSC();

	/**
	 * @brief Parses the MQTT message from the Sonoff SC and creates
	 * from it SensorData.
	 */
	SensorData parseMessage(const MqttMessage& message) override;
};

}
