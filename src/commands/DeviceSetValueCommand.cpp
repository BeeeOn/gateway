#include "commands/DeviceSetValueCommand.h"

using namespace BeeeOn;
using namespace std;

DeviceSetValueCommand::DeviceSetValueCommand(const DeviceID &deviceID, const ModuleID &moduleID, const double value,
		const Poco::Timespan &timeout) :
	m_deviceID(deviceID),
	m_moduleID(moduleID),
	m_value(value),
	m_timeout(timeout)
{
}

DeviceSetValueCommand::~DeviceSetValueCommand()
{
}

ModuleID DeviceSetValueCommand::moduleID() const
{
	return m_moduleID;
}

double DeviceSetValueCommand::value() const
{
	return m_value;
}

Poco::Timespan DeviceSetValueCommand::timeout() const
{
	return m_timeout;
}

DeviceID DeviceSetValueCommand::deviceID() const
{
	return m_deviceID;
}

string DeviceSetValueCommand::toString() const
{
	string cmdString;
	cmdString += name() + " ";
	cmdString += m_deviceID.toString() + " ";
	cmdString += m_moduleID.toString() + " ";
	cmdString += to_string(m_value) + " ";
	cmdString += to_string(m_timeout.totalSeconds()) + " ";

	return cmdString;
}
