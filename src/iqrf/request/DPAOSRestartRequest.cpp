#include "iqrf/request/DPAOSRestartRequest.h"

using namespace BeeeOn;

static const uint8_t RESTART_CMD = 0x08;

DPAOSRestartRequest::DPAOSRestartRequest(uint8_t node):
	DPARequest(
		node,
		DPA_OS_PNUM,
		RESTART_CMD
	)
{
}
