#include "iqrf/request/DPAOSPeripheralInfoRequest.h"

using namespace BeeeOn;

static const uint8_t PERIPHERAL_INFO_CMD = 0x00;

DPAOSPeripheralInfoRequest::DPAOSPeripheralInfoRequest(uint8_t node):
	DPARequest(
		node,
		DPA_OS_PNUM,
		PERIPHERAL_INFO_CMD
	)
{
}
