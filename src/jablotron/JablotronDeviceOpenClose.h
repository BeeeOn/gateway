#pragma once

#include "jablotron/JablotronDevice.h"

namespace BeeeOn {

class JablotronDeviceOpenClose : public JablotronDevice {
public:
	JablotronDeviceOpenClose(
		const DeviceID &deviceID, const std::string &name);

	/**
	 * Example of message:
	 *  [XXXXXXXX] XX-XXX SENSOR LB:? ACT:1
	 *  [XXXXXXXX] XX-XXX SENSOR LB:? ACT:0
	 *  [XXXXXXXX] XX-XXX TAMPER LB:? ACT:1
	 *  [XXXXXXXX] XX-XXX TAMPER LB:? ACT:0
	 *  [XXXXXXXX] XX-XXX BEACON LB:?
	 *
	 * XX-XXX: can be JA-81M or JA-83M
	 */
	SensorData extractSensorData(const std::string &message) override;

	std::list<ModuleType> moduleTypes() override;

	Poco::Timespan refreshTime() override;
};

}
