#include <Poco/Exception.h>

#include "core/Answer.h"

using namespace BeeeOn;
using namespace Poco;

Result::Result(Poco::AutoPtr<Answer> answer):
	m_status(Status::PENDING),
	m_answer(*answer)
{
	m_answer.addResult(this);
}

Result::~Result()
{
}

void Result::setStatus(const Result::Status status)
{
	ScopedLock guard(*this);

	if (m_status == status)
		return;

	if (status.raw() < m_status.raw()) {
		throw Poco::InvalidArgumentException(
			"invalid status change: " + status);
	}

	m_status = status;
	notifyUpdated();
}

Result::Status Result::status() const
{
	ScopedLock guard(const_cast<Result &>(*this));
	return m_status;
}

void Result::notifyUpdated()
{
	m_answer.notifyUpdated();
}

void Result::lock()
{
	m_answer.lock();
}

void Result::unlock()
{
	m_answer.unlock();
}

EnumHelper<BeeeOn::Result::StatusEnum::Raw>::ValueMap &Result::StatusEnum::valueMap()
{
	static EnumHelper<Result::Status::Raw>::ValueMap valueMap = {
		{Status::PENDING, "PENDING"},
		{Status::SUCCESS, "SUCCESS"},
		{Status::FAILED, "FAILED"},
	};

	return valueMap;
}
