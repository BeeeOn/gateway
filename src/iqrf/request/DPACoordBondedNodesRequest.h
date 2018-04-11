#pragma once

#include "iqrf/DPARequest.h"

namespace BeeeOn {

/**
 * @brief DPA message that requires list of paired devices
 * from coordinator.
 */
class DPACoordBondedNodesRequest final : public DPARequest {
public:
	typedef Poco::SharedPtr<DPACoordBondedNodesRequest> Ptr;

	DPACoordBondedNodesRequest();
};

}
