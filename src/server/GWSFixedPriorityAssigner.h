#pragma once

#include "server/GWSPriorityAssigner.h"

namespace BeeeOn {

/**
 * @brief Assigns a hard-wired priority to each message as follows:
 * - HIGHEST - responses, acks
 * - HIGH    - requests
 * - LOW     - others
 * - LOWEST  - sensor data export
 */
class GWSFixedPriorityAssigner : public GWSPriorityAssigner {
public:
	size_t assignPriority(const GWMessage::Ptr message) override;
};

}
