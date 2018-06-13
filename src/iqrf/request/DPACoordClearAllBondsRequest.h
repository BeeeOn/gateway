#pragma once

#include "iqrf/DPARequest.h"

namespace BeeeOn {

class DPACoordClearAllBondsRequest final : public DPARequest {
public:
	typedef Poco::SharedPtr<DPACoordClearAllBondsRequest> Ptr;

	DPACoordClearAllBondsRequest();
};

}
