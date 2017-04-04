#include "commands/DeviceUnpairCommand.h"

using namespace BeeeOn;

DeviceUnpairCommand::DeviceUnpairCommand(const DeviceID &deviceID):
	Command("DeviceUnpairCommand"),
	m_deviceID(deviceID)
{
}

DeviceUnpairCommand::~DeviceUnpairCommand()
{
}

DeviceID DeviceUnpairCommand::deviceID() const
{
	return m_deviceID;
}
