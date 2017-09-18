#include "commands/ServerLastValueCommand.h"

using namespace BeeeOn;

ServerLastValueCommand::ServerLastValueCommand(
		const DeviceID &deviceID, const ModuleID &moduleID):
	m_deviceID(deviceID),
	m_moduleID(moduleID)
{
}

ServerLastValueCommand::~ServerLastValueCommand()
{
}

DeviceID ServerLastValueCommand::deviceID() const
{
	return m_deviceID;
}

ModuleID ServerLastValueCommand::moduleID() const
{
	return m_moduleID;
}
