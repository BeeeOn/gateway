#include "commands/ServerLastValueCommand.h"

using namespace BeeeOn;
using namespace std;

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

string ServerLastValueCommand::toString() const
{
	string cmdString;
	cmdString += name() + " ";
	cmdString += m_deviceID.toString() + " ";
	cmdString += m_moduleID.toString();

	return cmdString;
}
