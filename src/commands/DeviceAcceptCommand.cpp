#include "commands/DeviceAcceptCommand.h"

using namespace BeeeOn;
using namespace std;

DeviceAcceptCommand::DeviceAcceptCommand(const DeviceID &deviceID):
	PrefixCommand(deviceID.prefix()),
	m_deviceID(deviceID)
{
}

DeviceAcceptCommand::~DeviceAcceptCommand()
{
}

DeviceID DeviceAcceptCommand::deviceID() const
{
	return m_deviceID;
}

string DeviceAcceptCommand::toString() const
{
	return name() + " " + m_deviceID.toString();
}
