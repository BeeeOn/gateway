#pragma once

#include "iqrf/DPARequest.h"

namespace BeeeOn {

class DPAOSRestartRequest final : public DPARequest {
public:
	typedef Poco::SharedPtr<DPAOSRestartRequest> Ptr;

	DPAOSRestartRequest(uint8_t node);
};

}
