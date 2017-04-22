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
