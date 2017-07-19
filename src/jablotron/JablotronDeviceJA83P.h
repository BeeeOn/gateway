#pragma once

#include "jablotron/JablotronDevice.h"

namespace BeeeOn {

class JablotronDeviceJA83P : public JablotronDevice {
public:
	JablotronDeviceJA83P(
		const DeviceID &deviceID, const std::string &name);

	/**
	 * Example of message:
	 *  [XXXXXXXX] XX-XXX SENSOR LB:?
	 *  [XXXXXXXX] XX-XXX TAMPER LB:? ACT:1
	 *  [XXXXXXXX] XX-XXX TAMPER LB:? ACT:0
	 *  [XXXXXXXX] XX-XXX BEACON LB:?
	 */
	SensorData extractSensorData(const std::string &message) override;

	std::list<ModuleType> moduleTypes() override;

	Poco::Timespan refreshTime() override;
};

}
