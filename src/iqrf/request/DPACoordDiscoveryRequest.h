#pragma once

#include "iqrf/DPARequest.h"

namespace BeeeOn {

class DPACoordDiscoveryRequest final : public DPARequest {
public:
	typedef Poco::SharedPtr<DPACoordDiscoveryRequest> Ptr;

	DPACoordDiscoveryRequest();
};

}
