#pragma once

#include "iqrf/DPARequest.h"

namespace BeeeOn {

class DPAOSPeripheralInfoRequest final : public DPARequest {
public:
	typedef Poco::SharedPtr<DPAOSPeripheralInfoRequest> Ptr;

	DPAOSPeripheralInfoRequest(uint8_t node);
};

}
