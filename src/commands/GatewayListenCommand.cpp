#include "commands/GatewayListenCommand.h"

using namespace BeeeOn;

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
