#include "core/AnswerQueue.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

AnswerQueue::AnswerQueue()
{
}

Answer::Ptr AnswerQueue::newAnswer()
{
	return new Answer(*this);
}

bool AnswerQueue::wait(const Timespan &timeout, list<Answer::Ptr> &dirtyList)
{
	do {
		list<Answer::Ptr> tmpList;

		listDirty(tmpList);

		if (!tmpList.empty()) {
			dirtyList = tmpList;
			return true;
		}
	} while (block(timeout));

	return false;
}

void AnswerQueue::listDirty(list<Answer::Ptr> &dirtyList) const
{
	FastMutex::ScopedLock lock(m_mutex);

	for (auto answer : m_answerList) {
		FastMutex::ScopedLock guard(answer->lock());
		if (answer->isDirtyUnlocked()) {
			dirtyList.push_back(answer);
			answer->setDirtyUnlocked(false);
		}
	}
}

void AnswerQueue::add(Answer *answer)
{
	FastMutex::ScopedLock lock(m_mutex);

	m_answerList.push_back(AutoPtr<Answer>(answer, true));
}

bool AnswerQueue::block(const Timespan &timeout)
{
	if (timeout.totalMicroseconds() == 0)
		return false;

	if (timeout.totalMicroseconds() < 0) {
		m_event.wait();
		return true;
	}

	return m_event.tryWait(timeout.totalMilliseconds());
}

void AnswerQueue::remove(const Answer::Ptr answer)
{
	FastMutex::ScopedLock lock(m_mutex);

	auto it = find(m_answerList.begin(), m_answerList.end(), answer);
	if (it != m_answerList.end())
		m_answerList.erase(it);
}

Event &AnswerQueue::event()
{
	return m_event;
}

unsigned long AnswerQueue::size() const
{
	FastMutex::ScopedLock lock(m_mutex);
	return m_answerList.size();
}
