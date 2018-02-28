#include <vector>

#include "core/Answer.h"
#include "core/AnswerQueue.h"

using namespace BeeeOn;
using namespace Poco;

Answer::Answer(AnswerQueue &answerQueue):
	m_answerQueue(answerQueue),
	m_dirty(0),
	m_handlers(0)
{
	answerQueue.add(this);
}

void Answer::setDirty(bool dirty)
{
	ScopedLock guard(*this);
	m_dirty = dirty;
}

bool Answer::isDirty() const
{
	ScopedLock guard(const_cast<Answer &>(*this));
	return (bool) m_dirty;
}

Event &Answer::event()
{
	return m_answerQueue.event();
}

bool Answer::isPending() const
{
	ScopedLock guard(const_cast<Answer &>(*this));

	if ((unsigned long) m_handlers != m_resultList.size())
		return true;

	if (m_handlers == 0)
		return false;

	for (auto &result : m_resultList) {
		if (result->status() == Result::Status::PENDING)
			return true;
	}

	return false;
}

bool Answer::isEmpty() const
{
	return m_handlers == 0;
}

unsigned long Answer::resultsCount() const
{
	ScopedLock guard(const_cast<Answer &>(*this));
	return m_resultList.size();
}

void Answer::addResult(Result *result)
{
	ScopedLock guard(*this);

	if (m_answerQueue.isDisposed()) {
		throw IllegalStateException(
			"inserting result into a disposed AnswerQueue");
	}

	if (m_resultList.size() >= m_handlers) {
		// addResult is probably called too late
		throw IllegalStateException(
			"no more room for results");
	}

	m_resultList.push_back(AutoPtr<Result>(result, true));
}

int Answer::handlersCount() const
{
	ScopedLock guard(const_cast<Answer &>(*this));
	return m_handlers;
}

void Answer::notifyUpdated()
{
	ScopedLock guard(*this);

	setDirty(true);
	event().set();
}

Result::Ptr Answer::at(size_t position)
{
	ScopedLock guard(*this);
	return m_resultList.at(position);
}

std::vector<Result::Ptr>::iterator Answer::begin()
{
	ScopedLock guard(*this);
	return m_resultList.begin();
}

std::vector<Result::Ptr>::iterator Answer::end()
{
	ScopedLock guard(*this);
	return m_resultList.end();
}

void Answer::installImpl(Poco::SharedPtr<AnswerImpl> answerImpl)
{
	m_answerImpl = answerImpl;
}

void Answer::setHandlersCount(unsigned long handlers)
{
	m_handlers = handlers;
}

void Answer::waitNotPending(const Poco::Timespan &timeout)
{
	// blocking wait
	if (timeout < 0) {
		while (isPending())
			event().wait();

		return;
	}

	// non-blocking wait with timeout
	Timestamp start;
	while (isPending()) {
		const Timespan remaining(timeout - start.elapsed());

		if (remaining <= 0)
			throw TimeoutException("timeout expired");

		event().wait(remaining.totalMilliseconds());
	}
}
