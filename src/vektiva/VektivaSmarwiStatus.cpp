#include "vektiva/VektivaSmarwiStatus.h"

using namespace Poco;
using namespace BeeeOn;

VektivaSmarwiStatus::VektivaSmarwiStatus(
		const int status,
		const int error,
		const int ok,
		const int ro,
		const bool pos,
		const int fix,
		const Net::IPAddress ipAddress,
		const int rssi) :
	m_status(status),
	m_error(error),
	m_ok(ok),
	m_ro(ro),
	m_pos(pos),
	m_fix(fix),
	m_ipAddress(ipAddress),
	m_rssi(rssi)
{
}

int VektivaSmarwiStatus::status() const
{
	return m_status;
}

int VektivaSmarwiStatus::error() const
{
	return m_error;
}

int VektivaSmarwiStatus::ok() const
{
	return m_ok;
}

int VektivaSmarwiStatus::ro() const
{
	return m_ro;
}

bool VektivaSmarwiStatus::pos() const
{
	return m_pos;
}

Net::IPAddress VektivaSmarwiStatus::ipAddress() const
{
	return m_ipAddress;
}

int VektivaSmarwiStatus::fix() const
{
	return m_fix;
}

int VektivaSmarwiStatus::rssi() const
{
	return m_rssi;
}
