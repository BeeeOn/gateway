#include "commands/DeviceAcceptCommand.h"

using namespace BeeeOn;

DeviceAcceptCommand::DeviceAcceptCommand(const DeviceID &deviceID):
	Command("DeviceAcceptCommand"),
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
