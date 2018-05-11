#include "core/PrefixCommand.h"

using namespace BeeeOn;

PrefixCommand::PrefixCommand(const DevicePrefix &prefix):
	m_prefix(prefix)
{
}

DevicePrefix PrefixCommand::prefix() const
{
	return m_prefix;
}
