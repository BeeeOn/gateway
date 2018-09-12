#pragma once

#include "util/Enum.h"

namespace BeeeOn {

class VPTBoilerModuleTypeEnum {
public:
	enum Raw {
		MOD_BOILER_STATUS = 0,
		MOD_BOILER_MODE,
		MOD_CURRENT_WATER_TEMPERATURE,
		MOD_CURRENT_OUTSIDE_TEMPERATURE,
		MOD_AVERAGE_OUTSIDE_TEMPERATURE,
		MOD_CURRENT_BOILER_PERFORMANCE,
		MOD_CURRENT_BOILER_PRESSURE,
		MOD_CURRENT_BOILER_OT_FAULT_FLAGS,
		MOD_CURRENT_BOILER_OT_OEM_FAULTS,
	};

	static EnumHelper<Raw>::ValueMap &valueMap();
};

typedef Enum<VPTBoilerModuleTypeEnum> VPTBoilerModuleType;

}
