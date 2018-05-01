#include "commands/DeviceUnpairCommand.h"
#include "commands/DeviceUnpairResult.h"

using namespace BeeeOn;
using namespace std;

DeviceUnpairCommand::DeviceUnpairCommand(const DeviceID &deviceID):
	PrefixCommand(deviceID.prefix()),
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

string DeviceUnpairCommand::toString() const
{
	return name() + " " + m_deviceID.toString();
}

Result::Ptr DeviceUnpairCommand::deriveResult(Answer::Ptr answer) const
{
	return new DeviceUnpairResult(answer);
}
