#include <Poco/Exception.h>

#include "commands/DeviceSearchCommand.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;
using namespace BeeeOn;

DeviceSearchCommand::DeviceSearchCommand(
		const DevicePrefix &prefix,
		const DeviceCriteria &criteria,
		const Timespan &duration):
	PrefixCommand(prefix),
	m_criteria(criteria),
	m_duration(duration)
{
}

DeviceCriteria DeviceSearchCommand::criteria() const
{
	return m_criteria;
}

bool DeviceSearchCommand::hasIPAddress() const
{
	return m_criteria.hasIPAddress();
}

Poco::Net::IPAddress DeviceSearchCommand::ipAddress() const
{
	return m_criteria.ipAddress();
}

bool DeviceSearchCommand::hasMACAddress() const
{
	return m_criteria.hasMACAddress();
}

MACAddress DeviceSearchCommand::macAddress() const
{
	return m_criteria.macAddress();
}

bool DeviceSearchCommand::hasSerialNumber() const
{
	return m_criteria.hasSerialNumber();
}

uint64_t DeviceSearchCommand::serialNumber() const
{
	return m_criteria.serialNumber();
}

Timespan DeviceSearchCommand::duration() const
{
	return m_duration;
}

string DeviceSearchCommand::toString() const
{
	return name()
		+ " " + prefix().toString();
		+ " " + to_string(m_duration.totalSeconds())
		+ ": " + m_criteria.toString();
}
