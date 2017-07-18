#pragma once

#include "jablotron/JablotronDevice.h"

namespace BeeeOn {

class JablotronDeviceAC88 : public JablotronDevice {
public:
	JablotronDeviceAC88(
		const DeviceID &deviceID, const std::string &name);

	/**
	 * Example:
	 *  [XXXXXXXX] AC-88 RELAY:?
	 */
	SensorData extractSensorData(const std::string &message) override;

	std::list<ModuleType> moduleTypes() override;
};

}
