#include "iqrf/request/DPACoordRemoveNodeRequest.h"

using namespace BeeeOn;

static const uint8_t UNBOND_ADDRESS_CMD = 0x05;

DPACoordRemoveNodeRequest::DPACoordRemoveNodeRequest(
		uint8_t node):
	DPARequest(
		COORDINATOR_NODE_ADDRESS,
		DPA_COORD_PNUM,
		UNBOND_ADDRESS_CMD,
		DEFAULT_HWPID,
		{node}
	)
{
}
