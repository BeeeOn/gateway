#include <vector>

#include "core/Answer.h"
#include "core/AnswerQueue.h"

using namespace BeeeOn;
using namespace Poco;

Answer::Answer(AnswerQueue &answerQueue):
	m_answerQueue(answerQueue),
	m_dirty(0)
{
	answerQueue.add(this);
}

void Answer::setDirty(bool dirty)
{
	FastMutex::ScopedLock guard(lock());
	setDirtyUnlocked(dirty);
}

void Answer::setDirtyUnlocked(bool dirty)
{
	assureLocked();
	m_dirty = dirty;
}

bool Answer::isDirty() const
{
	FastMutex::ScopedLock guard(lock());
	return isDirtyUnlocked();
}

bool Answer::isDirtyUnlocked() const
{
	assureLocked();
	return (bool) m_dirty;
}

Event &Answer::event()
{
	return m_answerQueue.event();
}

FastMutex &Answer::lock() const
{
	return m_lock;
}

bool Answer::isPending() const
{
	FastMutex::ScopedLock guard(m_lock);
	return isPendingUnlocked();
}

bool Answer::isPendingUnlocked() const
{
	assureLocked();

	if ((unsigned long) m_commands != m_resultList.size())
		return true;

	if (m_commands == 0)
		return false;

	for (auto &result : m_resultList) {
		if (result->statusUnlocked() == Result::PENDING)
			return true;
	}

	return false;
}

bool Answer::isEmpty() const
{
	return m_commands == 0;
}

unsigned long Answer::resultsCount() const
{
	FastMutex::ScopedLock guard(m_lock);
	return resultsCountUnlocked();
}

unsigned long Answer::resultsCountUnlocked() const
{
	return m_resultList.size();
}

void Answer::addResult(Result *result)
{
	FastMutex::ScopedLock guard(m_lock);
	m_resultList.push_back(AutoPtr<Result>(result, true));
}

int Answer::commandsCount() const
{
	FastMutex::ScopedLock guard(m_lock);
	return commandsCountUnlocked();
}

int Answer::commandsCountUnlocked() const
{
	return m_commands;
}

void Answer::assureLocked() const
{
	if (lock().tryLock()) {
		lock().unlock();
		throw IllegalStateException(
			"modifying unlocked Answer");
	}
}

void Answer::notifyUpdated()
{
	assureLocked();
	setDirtyUnlocked(true);
	event().set();
}

Result::Ptr Answer::at(size_t position)
{
	FastMutex::ScopedLock guard(m_lock);
	return atUnlocked(position);
}

Result::Ptr Answer::atUnlocked(size_t position)
{
	assureLocked();
	return m_resultList.at(position);
}

std::vector<Result::Ptr>::iterator Answer::begin()
{
	assureLocked();
	return m_resultList.begin();
}

std::vector<Result::Ptr>::iterator Answer::end()
{
	assureLocked();
	return m_resultList.end();
}
