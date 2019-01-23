#include "di/Injectable.h"
#include "server/GWSFixedPriorityAssigner.h"

BEEEON_OBJECT_BEGIN(BeeeOn, GWSFixedPriorityAssigner)
BEEEON_OBJECT_CASTABLE(GWSPriorityAssigner)
BEEEON_OBJECT_END(BeeeOn, GWSFixedPriorityAssigner)

using namespace BeeeOn;

static const size_t RESPONSE_PRIORITY = 0;
static const size_t REQUEST_PRIORITY  = 1;
static const size_t DATA_PRIORITY     = 3;
static const size_t OTHERS_PRIORITY   = 2;

size_t GWSFixedPriorityAssigner::assignPriority(const GWMessage::Ptr message)
{
	switch (message->type()) {
	case GWMessageType::GENERIC_RESPONSE:
	case GWMessageType::GENERIC_ACK:
	case GWMessageType::RESPONSE_WITH_ACK:
	case GWMessageType::UNPAIR_RESPONSE:
		return RESPONSE_PRIORITY;

	case GWMessageType::DEVICE_ACCEPT_REQUEST:
	case GWMessageType::DEVICE_LIST_REQUEST:
	case GWMessageType::LAST_VALUE_REQUEST:
	case GWMessageType::LISTEN_REQUEST:
	case GWMessageType::NEW_DEVICE_REQUEST:
	case GWMessageType::NEW_DEVICE_GROUP_REQUEST:
	case GWMessageType::SET_VALUE_REQUEST:
	case GWMessageType::UNPAIR_REQUEST:
		return REQUEST_PRIORITY;

	case GWMessageType::SENSOR_DATA_EXPORT:
		return DATA_PRIORITY;

	default:
		return OTHERS_PRIORITY;
	}
}
