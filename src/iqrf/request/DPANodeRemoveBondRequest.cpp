#include "iqrf/request/DPANodeRemoveBondRequest.h"

using namespace BeeeOn;

static const uint8_t REMOVE_BOND_CMD = 0x01;

DPANodeRemoveBondRequest::DPANodeRemoveBondRequest(uint8_t node):
	DPARequest(
		node,
		DPA_NODE_PNUM,
		REMOVE_BOND_CMD
	)
{
}
