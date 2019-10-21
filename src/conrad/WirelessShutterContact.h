#pragma once

#include <list>

#include <Poco/SharedPtr.h>

#include "conrad/ConradDevice.h"
#include "model/SensorData.h"

namespace BeeeOn {

/**
 * @brief The class represents a standalone device Conrad Wireless shutter
 * contact. It allows to communicate with the device via Conrad interface.
 */
class WirelessShutterContact : public ConradDevice {
public:
	typedef Poco::SharedPtr<WirelessShutterContact> Ptr;

	static const std::string PRODUCT_NAME;

	WirelessShutterContact(const DeviceID& id, const RefreshTime &refresh);
	~WirelessShutterContact();

	/**
	 * @brief Parses the message from Conrad interface and creates
	 * from it SensorData.
	 */
	SensorData parseMessage(const Poco::JSON::Object::Ptr message) override;
};

}
