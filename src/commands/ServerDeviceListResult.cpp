#include "commands/ServerDeviceListResult.h"

using namespace Poco;
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

	const ModuleValues empty;

	for (const auto &id : deviceList)
		m_data.emplace(id, empty);
}

vector<DeviceID> ServerDeviceListResult::deviceList() const
{
	ScopedLock guard(const_cast<ServerDeviceListResult &>(*this));

	vector<DeviceID> ids;

	for (const auto &pair : m_data)
		ids.emplace_back(pair.first);

	return ids;
}

void ServerDeviceListResult::setDevices(const DeviceValues &values)
{
	ScopedLock guard(*this);

	m_data = values;
}

ServerDeviceListResult::DeviceValues ServerDeviceListResult::devices() const
{
	ScopedLock guard(const_cast<ServerDeviceListResult &>(*this));

	return m_data;
}

Nullable<double> ServerDeviceListResult::value(
		const DeviceID &id,
		const ModuleID &module) const
{
	ScopedLock guard(const_cast<ServerDeviceListResult &>(*this));

	Nullable<double> result;
	auto it = m_data.find(id);
	if (it == m_data.end())
		return result;

	auto mit = it->second.find(module);
	if (mit == it->second.end())
		return result;

	result = mit->second;
	return result;
}
