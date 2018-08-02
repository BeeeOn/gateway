#include "iqrf/request/DPACoordBondedNodesRequest.h"

using namespace BeeeOn;

static const uint8_t BONDED_NODES_CMD = 0x02;

DPACoordBondedNodesRequest::DPACoordBondedNodesRequest():
	DPARequest(
		COORDINATOR_NODE_ADDRESS,
		DPA_COORD_PNUM,
		BONDED_NODES_CMD
	)
{
}
