#include "commands/ServerDeviceListResult.h"

using namespace BeeeOn;
using namespace std;

ServerDeviceListResult::ServerDeviceListResult(
		const Answer::Ptr answer):
	Result(answer)
{
}

ServerDeviceListResult::~ServerDeviceListResult()
{
}

void ServerDeviceListResult::setDeviceList(
	const vector<DeviceID> &deviceList)
{
	Poco::FastMutex::ScopedLock guard(lock());
	m_deviceList = deviceList;
}

void ServerDeviceListResult::setDeviceListUnlocked(
	const vector<DeviceID> &deviceList)
{
	assureLocked();
	m_deviceList = deviceList;
}

vector<DeviceID> ServerDeviceListResult::deviceList() const
{
	Poco::FastMutex::ScopedLock guard(lock());
	return deviceListUnlocked();
}

vector<DeviceID> ServerDeviceListResult::deviceListUnlocked() const
{
	assureLocked();
	return m_deviceList;
}
