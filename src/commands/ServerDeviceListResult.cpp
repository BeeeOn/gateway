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
	ScopedLock guard(*this);
	m_deviceList = deviceList;
}

vector<DeviceID> ServerDeviceListResult::deviceList() const
{
	ScopedLock guard(const_cast<ServerDeviceListResult &>(*this));
	return m_deviceList;
}
