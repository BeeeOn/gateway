#pragma once

#include "zwave/ZWaveDeviceInfo.h"

namespace BeeeOn {

class DefaultZWaveDeviceInfo : public ZWaveDeviceInfo {
public:
	DefaultZWaveDeviceInfo();

	std::string convertValue(double value) override;
};

}
