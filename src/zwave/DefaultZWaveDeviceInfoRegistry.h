#pragma once

#include "zwave/ZWaveDeviceInfoRegistry.h"

namespace BeeeOn {

class DefaultZWaveDeviceInfoRegistry : public ZWaveDeviceInfoRegistry {
public:
	ZWaveDeviceInfo::Ptr find(
		uint32_t manufacturer, uint32_t product) override;
};

}
