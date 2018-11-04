#pragma once

#include <set>

#include "iqrf/DPAResponse.h"

namespace BeeeOn {

/**
 * @brief DPA message that contains list of paired devices
 * from coordinator. Up to 256 devices can be bonded.
 */
class DPACoordBondedNodesResponse final : public DPAResponse {
public:
	typedef Poco::SharedPtr<DPACoordBondedNodesResponse> Ptr;

	/**
	 * @brief Each node id of device is stored as bit index.
	 *
	 * Example:
	 *
	 *  - PData: FE.01.00.00... (32B)
	 *
	 * List of node id:
	 *
	 *  - 1 = 0 * 8 + 1
	 *  - 2 = 0 * 8 + 2
	 *  - 3 = 0 * 8 + 3
	 *  - ...
	 *  - 8 = 1 * 8 + 0
	 */
	std::set<uint8_t> decodeNodeBonded() const;
};

}
