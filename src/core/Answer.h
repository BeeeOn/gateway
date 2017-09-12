#ifndef BEEEON_ANSWER_H
#define BEEEON_ANSWER_H

#include <Poco/AutoPtr.h>
#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/RefCountedObject.h>
#include <Poco/Task.h>
#include <Poco/TaskManager.h>

#include "core/AnswerImpl.h"
#include "core/Command.h"
#include "core/CommandProgressHandler.h"
#include "core/Result.h"

namespace BeeeOn {

class AnswerQueue;
class Command;
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
 */
class Answer : public Poco::RefCountedObject {
	friend CommandDispatcher;
public:
	typedef Poco::AutoPtr<Answer> Ptr;

	Answer(AnswerQueue &answerQueue);

	/*
	 * All reference counted objects should have a protected destructor,
	 * to forbid explicit use of delete.
	 */
	Answer(const Answer&) = delete;

	/*
	 * The status that informs about the change of a Result.
	 */
	void setDirty(bool dirty);
	void setDirtyUnlocked(bool dirty);

	bool isDirty() const;
	bool isDirtyUnlocked() const;

	/*
	 * The check if the Result are in the terminal state (SUCCESS/ERROR).
	 */
	bool isPending() const;
	bool isPendingUnlocked() const;

	Poco::Event &event();
	Poco::FastMutex &lock() const;

	/*
	 * True if the list of commands is empty.
	 */
	bool isEmpty() const;

	unsigned long resultsCount() const;
	unsigned long resultsCountUnlocked() const;

	int handlersCount() const;
	int handlersCountUnlocked() const;
	void setHandlersCount(unsigned long counter);

	void addResult(Result *result);
	void addResultUnlocked(Result *result);

	/*
	 * Notifies the waiting queue that this Answer isEmpty().
	 * The call sets Answer::setDirtyUnlocked(true).
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
	Result::Ptr atUnlocked(size_t position);

	std::vector<Result::Ptr>::iterator begin();
	std::vector<Result::Ptr>::iterator end();

protected:
	/*
	 * The check if the operation is locked.
	 */
	void assureLocked() const;

	void installImpl(Poco::SharedPtr<AnswerImpl> answerImpl);

private:
	AnswerQueue &m_answerQueue;
	Poco::AtomicCounter m_dirty;
	mutable Poco::FastMutex m_lock;
	std::vector<Result::Ptr> m_resultList;
	unsigned long m_handlers;
	AnswerImpl::Ptr m_answerImpl;
};

}

#endif
