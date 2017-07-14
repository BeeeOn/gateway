#include "di/Injectable.h"
#include "zwave/DefaultZWaveDeviceInfoRegistry.h"
#include "zwave/DefaultZWaveDeviceInfo.h"

BEEEON_OBJECT_BEGIN(BeeeOn, DefaultZWaveDeviceInfoRegistry)
BEEEON_OBJECT_CASTABLE(ZWaveDeviceInfoRegistry)
BEEEON_OBJECT_END(BeeeOn, DefaultZWaveDeviceInfoRegistry)

using namespace BeeeOn;

static const ZWaveDeviceInfo::Ptr defaultDeviceInfo = new DefaultZWaveDeviceInfo();

ZWaveDeviceInfo::Ptr DefaultZWaveDeviceInfoRegistry::find(uint32_t, uint32_t)
{
	return defaultDeviceInfo;
}
