#include "commands/ServerLastValueResult.h"

using namespace BeeeOn;

ServerLastValueResult::ServerLastValueResult(
	const Answer::Ptr answer):
	Result(answer)
{
}

ServerLastValueResult::~ServerLastValueResult()
{
}

void ServerLastValueResult::setValue(double value)
{
	ScopedLock guard(*this);
	m_value = value;
}

double ServerLastValueResult::value() const
{
	ScopedLock guard(const_cast<ServerLastValueResult &>(*this));
	return m_value;
}

void ServerLastValueResult::setDeviceID(const DeviceID &deviceID)
{
	ScopedLock guard(*this);
	m_deviceID = deviceID;
}

DeviceID ServerLastValueResult::deviceID() const
{
	ScopedLock guard(const_cast<ServerLastValueResult &>(*this));
	return m_deviceID;
}

void ServerLastValueResult::setModuleID(const ModuleID &moduleID)
{
	ScopedLock guard(*this);
	m_moduleID = moduleID;
}

ModuleID ServerLastValueResult::moduleID() const
{
	ScopedLock guard(const_cast<ServerLastValueResult &>(*this));
	return  m_moduleID;
}
