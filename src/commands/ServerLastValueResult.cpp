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
	Poco::FastMutex::ScopedLock guard(lock());
	setValueUnlocked(value);
}

void ServerLastValueResult::setValueUnlocked(double value)
{
	assureLocked();
	m_value = value;
}

double ServerLastValueResult::value() const
{
	Poco::FastMutex::ScopedLock guard(lock());
	return valueUnlocked();
}

double ServerLastValueResult::valueUnlocked() const
{
	assureLocked();
	return m_value;
}

void ServerLastValueResult::setDeviceID(const DeviceID &deviceID)
{
	Poco::FastMutex::ScopedLock guard(lock());
	setDeviceIDUnlocked(deviceID);
}

void ServerLastValueResult::setDeviceIDUnlocked(const DeviceID &deviceID)
{
	assureLocked();
	m_deviceID = deviceID;
}

DeviceID ServerLastValueResult::deviceID() const
{
	Poco::FastMutex::ScopedLock guard(lock());
	return deviceIDUnlocked();
}

DeviceID ServerLastValueResult::deviceIDUnlocked() const
{
	assureLocked();
	return m_deviceID;
}

void ServerLastValueResult::setModuleID(const ModuleID &moduleID)
{
	Poco::FastMutex::ScopedLock guard(lock());
	setModuleIDUnlocked(moduleID);
}

void ServerLastValueResult::setModuleIDUnlocked(const ModuleID &moduleID)
{
	assureLocked();
	m_moduleID = moduleID;
}

ModuleID ServerLastValueResult::moduleID() const
{
	Poco::FastMutex::ScopedLock guard(lock());
	return moduleIDUnlocked();
}

ModuleID ServerLastValueResult::moduleIDUnlocked() const
{
	assureLocked();
	return  m_moduleID;
}
