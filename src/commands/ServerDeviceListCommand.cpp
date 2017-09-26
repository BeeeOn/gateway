#include "commands/ServerDeviceListCommand.h"

using namespace BeeeOn;
using namespace std;

ServerDeviceListCommand::ServerDeviceListCommand(
		const DevicePrefix &prefix):
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

string ServerDeviceListCommand::toString() const
{
	return name() + " " +  m_prefix.toString();
}
