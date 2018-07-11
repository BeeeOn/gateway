#include "commands/DeviceUnpairCommand.h"
#include "commands/DeviceUnpairResult.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

DeviceUnpairCommand::DeviceUnpairCommand(
		const DeviceID &deviceID,
		const Timespan &timeout):
	PrefixCommand(deviceID.prefix()),
	m_deviceID(deviceID),
	m_timeout(timeout)
{
}

DeviceUnpairCommand::~DeviceUnpairCommand()
{
}

DeviceID DeviceUnpairCommand::deviceID() const
{
	return m_deviceID;
}

Timespan DeviceUnpairCommand::timeout() const
{
	return m_timeout;
}

string DeviceUnpairCommand::toString() const
{
	return name() + " " + m_deviceID.toString();
}

Result::Ptr DeviceUnpairCommand::deriveResult(Answer::Ptr answer) const
{
	return new DeviceUnpairResult(answer);
}
