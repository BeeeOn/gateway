#pragma once

#include "jablotron/JablotronDevice.h"

namespace BeeeOn {

class JablotronDeviceTP82N : public JablotronDevice {
public:
	JablotronDeviceTP82N(const DeviceID &deviceID);

	/**
	 * Example of message:
	 *  [XXXXXXXX] TP-82N SET:##.#°C LB:?
	 *  [XXXXXXXX] TP-82N INT:##.#°C LB:?
	 */
	SensorData extractSensorData(const std::string &message) override;

	std::list<ModuleType> moduleTypes() override;
};

}
