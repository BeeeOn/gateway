#include "iqrf/request/DPACoordBondNodeRequest.h"

using namespace BeeeOn;

static const uint8_t BONDED_NODE_CMD = 0x04;

DPACoordBondNodeRequest::DPACoordBondNodeRequest():
	DPARequest(
		COORDINATOR_NODE_ADDRESS,
		DPA_COORD_PNUM,
		BONDED_NODE_CMD,
		DEFAULT_HWPID,
		{0x00, 0x00}
	)
{
}
