#include "iqrf/request/DPACoordDiscoveryRequest.h"

using namespace BeeeOn;

static const uint8_t DISCOVERY_NODE_CMD = 0x07;

DPACoordDiscoveryRequest::DPACoordDiscoveryRequest():
	DPARequest(
		COORDINATOR_NODE_ADDRESS,
		DPA_COORD_PNUM,
		DISCOVERY_NODE_CMD,
		DEFAULT_HWPID,
		{0x07, 0x00}
	)
{
}
