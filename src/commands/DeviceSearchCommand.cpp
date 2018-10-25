#include <Poco/Exception.h>

#include "commands/DeviceSearchCommand.h"

using namespace std;
using namespace Poco;
using namespace Poco::Net;
using namespace BeeeOn;

DeviceSearchCommand::DeviceSearchCommand(
		const DevicePrefix &prefix,
		const IPAddress &address,
		const Timespan &duration):
	PrefixCommand(prefix),
	m_ipAddress(address),
	m_duration(duration)
{
}

DeviceSearchCommand::DeviceSearchCommand(
		const DevicePrefix &prefix,
		const MACAddress &address,
		const Timespan &duration):
	PrefixCommand(prefix),
	m_macAddress(address),
	m_duration(duration)
{
}

DeviceSearchCommand::DeviceSearchCommand(
		const DevicePrefix &prefix,
		const uint64_t serialNumber,
		const Timespan &duration):
	PrefixCommand(prefix),
	m_serialNumber(serialNumber),
	m_duration(duration)
{
}

bool DeviceSearchCommand::hasIPAddress() const
{
	return !m_ipAddress.isNull();
}

Poco::Net::IPAddress DeviceSearchCommand::ipAddress() const
{
	return m_ipAddress;
}

bool DeviceSearchCommand::hasMACAddress() const
{
	return !m_macAddress.isNull();
}

MACAddress DeviceSearchCommand::macAddress() const
{
	return m_macAddress;
}

bool DeviceSearchCommand::hasSerialNumber() const
{
	return !m_serialNumber.isNull();
}

uint64_t DeviceSearchCommand::serialNumber() const
{
	return m_serialNumber;
}

Timespan DeviceSearchCommand::duration() const
{
	return m_duration;
}

string DeviceSearchCommand::toString() const
{
	const auto repr = name()
		+ " " + prefix().toString();
		+ " " + to_string(m_duration.totalSeconds());

	if (!m_ipAddress.isNull())
		return repr + ": " + m_ipAddress.value().toString();
	if (!m_macAddress.isNull())
		return repr + ": " + m_macAddress.value().toString();
	if (!m_serialNumber.isNull())
		return repr + ": " + to_string(m_serialNumber.value());

	throw IllegalStateException("no search criteria");
}
