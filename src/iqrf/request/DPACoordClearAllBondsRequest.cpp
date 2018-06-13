#include "iqrf/request/DPACoordClearAllBondsRequest.h"

using namespace BeeeOn;

static const uint8_t CLEAR_ALL_BONDS_CMD = 0x03;

DPACoordClearAllBondsRequest::DPACoordClearAllBondsRequest():
	DPARequest(
		COORDINATOR_NODE_ADDRESS,
		DPA_COORD_PNUM,
		CLEAR_ALL_BONDS_CMD
	)
{
}
