#ifndef BEEEON_ANSWER_H
#define BEEEON_ANSWER_H

#include <Poco/AutoPtr.h>
#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/RefCountedObject.h>
#include <Poco/SynchronizedObject.h>
#include <Poco/Task.h>
#include <Poco/TaskManager.h>

#include "core/Result.h"

namespace BeeeOn {

class AnswerQueue;
class CommandDispatcher;
class CommandHandler;
class Result;

/*
 * During the Answer creation the queue is set. The queue is notified using
 * the event() in the case of status change of the dirty.
 * The change of the status in Result causes this notification.
 *
 * The Answer and the Result share the common mutex. The operations that
 * change the status in the Answer and in the Result MUST be locked.
 *
 * Be aware, only a single thread is allowed to wait for notification (e.g.: via waitNotPending()).
 * Otherwise, a race condition can occur.
 */
class Answer : public Poco::RefCountedObject, public Poco::SynchronizedObject {
public:
	typedef Poco::AutoPtr<Answer> Ptr;

	Answer(AnswerQueue &answerQueue, const bool autoDispose = false);

	/*
	 * All reference counted objects should have a protected destructor,
	 * to forbid explicit use of delete.
	 */
	Answer(const Answer&) = delete;

	/*
	 * The status that informs about the change of a Result.
	 */
	void setDirty(bool dirty);
	bool isDirty() const;

	/*
	 * The check if the Result are in the terminal state (SUCCESS/ERROR).
	 */
	bool isPending() const;

	Poco::Event &event();

	/*
	 * True if the list of commands is empty.
	 */
	bool isEmpty() const;

	unsigned long resultsCount() const;

	int handlersCount() const;
	void setHandlersCount(unsigned long counter);

	void addResult(Result *result);

	/*
	 * Notifies the waiting queue that this Answer isEmpty().
	 * The call sets Answer::setDirty(true).
	 */
	void notifyUpdated();

	/**
	 * Waiting for the Answer in which Results aren't in PENDING
	 * state. Waiting can be blocking or non-blocking.
	 * Type of waiting is given by timeout. If timeout is less than 0,
	 * waiting is blocking, non-blocking otherwise. If timeout
	 * is non-blocking and Answer will not be signalled within
	 * the specified time interval, TimeoutException is thrown.
	 */
	void waitNotPending(const Poco::Timespan &timeout);

	Result::Ptr at(size_t position);

	std::vector<Result::Ptr>::iterator begin();
	std::vector<Result::Ptr>::iterator end();

private:
	AnswerQueue &m_answerQueue;
	Poco::Event m_notifyEvent;
	Poco::AtomicCounter m_dirty;
	std::vector<Result::Ptr> m_resultList;
	unsigned long m_handlers;
	const bool m_autoDispose;
};

}

#endif
