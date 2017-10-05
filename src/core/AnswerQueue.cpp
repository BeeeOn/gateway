#include <Poco/NumberFormatter.h>

#include "core/AnswerQueue.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

AnswerQueue::AnswerQueue():
	m_disposed(false)
{
}

Answer::Ptr AnswerQueue::newAnswer()
{
	if (m_disposed) {
		throw IllegalStateException(
			"creating Answer for a disposed AnswerQueue");
	}

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
	} while (block(timeout) && !m_disposed);

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

	if (m_disposed) {
		throw IllegalStateException(
			"adding Answer into a disposed AnswerQueue");
	}

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

void AnswerQueue::dispose()
{
	FastMutex::ScopedLock guard(m_mutex);

	for (auto &answer : m_answerList) {
		FastMutex::ScopedLock guard(answer->lock());

		int resultCount = answer->resultsCountUnlocked();
		const int handlersCount = answer->handlersCountUnlocked();
		const int missingCount = handlersCount - resultCount;

		string answerAddr =
			NumberFormatter::formatHex(reinterpret_cast<size_t>(answer.get()), true);

		if (missingCount > 0) {
			logger().debug(
				"finalizing Answer "
				+ answerAddr
				+ ", missing result "
				+ to_string(missingCount)
				+ "/"
				+ to_string(handlersCount),
				__FILE__, __LINE__);
		}

		for (int i = 0; i < missingCount; ++i) {
			new Result(answer, true);

			logger().debug(
				"created result for Answer "
				+ answerAddr
				+ ", "
				+ to_string(i + 1)
				+ "/"
				+ to_string(missingCount)
				+ " missing result",
				__FILE__, __LINE__);
		}

		resultCount = answer->resultsCountUnlocked();
		for (int i = 0; i < resultCount; ++i) {
			if (answer->atUnlocked(i)->statusUnlocked() == Result::Status::PENDING)
				answer->atUnlocked(i)->setStatusUnlocked(Result::Status::FAILED);

			logger().debug(
				"result "
				+ to_string(i + 1)
				+ "/"
				+ to_string(resultCount)
				+ " for Answer "
				+ answerAddr
				+ " done: "
				+ answer->atUnlocked(i)->statusUnlocked(),
				__FILE__, __LINE__);
		}
	}

	m_answerList.clear();
	m_disposed = true;
}

bool AnswerQueue::isDisposed() const
{
	return m_disposed;
}
