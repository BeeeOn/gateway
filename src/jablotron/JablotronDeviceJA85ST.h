#pragma once

#include "jablotron/JablotronDevice.h"

namespace BeeeOn {

class JablotronDeviceJA85ST : public JablotronDevice {
public:
	JablotronDeviceJA85ST(
		const DeviceID &deviceID, const std::string &name);

	/**
	 * Example:
	 *  [XXXXXXXX] JA-85ST SENSOR LB:?
	 *  [XXXXXXXX] JA-85ST BUTTON LB:?
	 *  [XXXXXXXX] JA-85ST TAMPER LB:? ACT:1
	 *  [XXXXXXXX] JA-85ST TAMPER LB:? ACT:0
	 *  [XXXXXXXX] JA-85ST DEFECT LB:? ACT:1
	 *  [XXXXXXXX] JA-85ST DEFECT LB:? ACT:0
	 *  [XXXXXXXX] JA-85ST BEACON LB:?
	 *  [XXXXXXXX] AC-88 RELAY:?
	 */
	SensorData extractSensorData(const std::string &message) override;

	std::list<ModuleType> moduleTypes() override;

	Poco::Timespan refreshTime() override;
};

}
