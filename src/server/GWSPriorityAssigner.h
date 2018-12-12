#pragma once

#include <Poco/SharedPtr.h>

#include "gwmessage/GWMessage.h"

namespace BeeeOn {

/**
 * @brief Implementation of GWSPriorityAssigner interface provides a method
 * for assigning of sending priority to GWMessage instances. The priorities
 * are used for better control flow of output traffic.
 *
 * The highest priority is 0 (the most urgent messages). Priorities greater
 * then 0 are lower (less urgent).
 */
class GWSPriorityAssigner {
public:
	typedef Poco::SharedPtr<GWSPriorityAssigner> Ptr;

	virtual ~GWSPriorityAssigner();

	/**
	 * @returns priority for the given message (smaller is better).
	 */
	virtual size_t assignPriority(const GWMessage::Ptr message) = 0;
};

}
