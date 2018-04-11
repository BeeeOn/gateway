#include "iqrf/response/DPACoordBondedNodesResponse.h"

using namespace BeeeOn;
using namespace std;

static const int DPA_BONDED_NODES_RESPONSE_SIZE = 32;

set<uint8_t> DPACoordBondedNodesResponse::decodeNodeBonded() const
{
	set<uint8_t> nodes;

	if (peripheralData().size() != DPA_BONDED_NODES_RESPONSE_SIZE) {
		throw Poco::RangeException(
			"data contained in DPA bonded nodes response has invalid size "
			+ to_string(peripheralData().size())
		);
	}

	for (size_t i = 0; i < DPA_BONDED_NODES_RESPONSE_SIZE; i++) {
		for (int j = 0; j < 8; j++) {
			if (((peripheralData()[i] >> j) &0x01) != 0)
				nodes.emplace(i * 8 + j);
		}
	}

	return nodes;
}
