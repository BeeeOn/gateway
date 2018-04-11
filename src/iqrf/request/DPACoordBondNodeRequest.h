#pragma once

#include "iqrf/DPARequest.h"

namespace BeeeOn {

/**
 * @brief Request for node bonding with 10 s timeout
 * (internal IQRF OS timeout).
 */
class DPACoordBondNodeRequest final : public DPARequest {
public:
	typedef Poco::SharedPtr<DPACoordBondNodeRequest> Ptr;

	DPACoordBondNodeRequest();
};

}
