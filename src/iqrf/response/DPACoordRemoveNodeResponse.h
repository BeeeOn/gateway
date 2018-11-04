#pragma once

#include "iqrf/DPAResponse.h"

namespace BeeeOn {

class DPACoordRemoveNodeResponse final : public DPAResponse {
public:
	typedef Poco::SharedPtr<DPACoordRemoveNodeResponse> Ptr;

	/**
	 * @returns Number of bonded nodes on the coordinator.
	 */
	size_t count() const;
};

}
