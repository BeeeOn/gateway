#pragma once

#include "iqrf/DPARequest.h"

namespace BeeeOn {

class DPANodeRemoveBondRequest final : public DPARequest {
public:
	typedef Poco::SharedPtr<DPANodeRemoveBondRequest> Ptr;

	DPANodeRemoveBondRequest(uint8_t node);
};

}
