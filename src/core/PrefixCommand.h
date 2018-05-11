#pragma once

#include "core/Command.h"
#include "model/DevicePrefix.h"

namespace BeeeOn {

/**
 * @brief Ancestor for Commands that are related to
 * a specific DevicePrefix only.
 */
class PrefixCommand : public Command {
public:
	PrefixCommand(const DevicePrefix &prefix);

	DevicePrefix prefix() const;

private:
	DevicePrefix m_prefix;
};

}
