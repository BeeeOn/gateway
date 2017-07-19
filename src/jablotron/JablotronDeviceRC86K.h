#pragma once

#include <jablotron/JablotronDevice.h>

namespace BeeeOn {

class JablotronDeviceRC86K : public JablotronDevice {
public:
	JablotronDeviceRC86K(
		const DeviceID &deviceID, const std::string &name);

	/**
	 * Example:
	 *  [XXXXXXXX] RC-86K ARM:1 LB:?
	 *  [XXXXXXXX] RC-86K ARM:0 LB:?
	 *  [XXXXXXXX] RC-86K PANIC LB:?
	 */
	SensorData extractSensorData(const std::string &message) override;

	std::list<ModuleType> moduleTypes() override;
};

}
