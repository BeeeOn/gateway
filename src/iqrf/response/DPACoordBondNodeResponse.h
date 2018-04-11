#pragma once

#include "iqrf/DPAResponse.h"

namespace BeeeOn {

class DPACoordBondNodeResponse final : public DPAResponse {
public:
	typedef Poco::SharedPtr<DPACoordBondNodeResponse> Ptr;

	/**
	 * @brief Returns address of the last bonded node.
	 */
	NetworkAddress bondedNetworkAddress() const;

	/**
	 * @brief Number of bonded nodes on the coordinator.
	 */
	size_t count() const;
};

}
