#pragma once

#include "iqrf/DPARequest.h"

namespace BeeeOn {

class DPACoordRemoveNodeRequest final : public DPARequest {
public:
	typedef Poco::SharedPtr<DPACoordRemoveNodeRequest> Ptr;

	DPACoordRemoveNodeRequest(uint8_t node);
};

}
