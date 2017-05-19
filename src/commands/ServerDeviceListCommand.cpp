#include "commands/ServerDeviceListCommand.h"

using namespace BeeeOn;

ServerDeviceListCommand::ServerDeviceListCommand(
		const DevicePrefix &prefix):
	Command("ServerDeviceListCommand"),
	m_prefix(prefix)
{
}

ServerDeviceListCommand::~ServerDeviceListCommand()
{
}

DevicePrefix ServerDeviceListCommand::devicePrefix() const
{
	return m_prefix;
}
