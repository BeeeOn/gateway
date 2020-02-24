#pragma once

#include <Poco/SharedPtr.h>

#include "conrad/ConradDevice.h"
#include "model/SensorData.h"

namespace BeeeOn {

/**
 * @brief The class represents a standalone device Conrad Radiator thermostat.
 * It allows to communicate with the device via Conrad interface.
 */
class RadiatorThermostat : public ConradDevice {
public:
	typedef Poco::SharedPtr<RadiatorThermostat> Ptr;

	static const std::string PRODUCT_NAME;

	RadiatorThermostat(const DeviceID& id, const RefreshTime &refresh);
	~RadiatorThermostat();

	void requestModifyState(
		const ModuleID& moduleID,
		const double value,
		FHEMClient::Ptr fhemClient) override;

	/**
	 * @brief Parses the message from Conrad interface and creates
	 * from it SensorData.
	 */
	SensorData parseMessage(const Poco::JSON::Object::Ptr message) override;
};

}
