#include "commands/GatewayListenCommand.h"

using namespace BeeeOn;
using namespace std;

GatewayListenCommand::GatewayListenCommand(const Poco::Timespan &duration):
	m_duration(duration)
{
}

GatewayListenCommand::~GatewayListenCommand()
{
}

Poco::Timespan GatewayListenCommand::duration() const
{
	return m_duration;
}

string GatewayListenCommand::toString() const
{
	return name() + " " + to_string(m_duration.totalSeconds()) + " ";
}
