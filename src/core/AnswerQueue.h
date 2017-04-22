#ifndef BEEEON_ANSWER_QUEUE_H
#define BEEEON_ANSWER_QUEUE_H

#include <list>

#include <Poco/Event.h>
#include <Poco/Timespan.h>

#include "core/Answer.h"

namespace BeeeOn {

/*
 * The responses are added to the queue during the Answer creation. After
 * the status is changed in the Result, the Answer is set to dirty and the
 * Answer notifies the queue about this change.
 *
 * It is possible to wait for Answer from queue for a given time using
 * wait(Timespan, dirtyList). After a given time Answers with the set dirty
 * (the status Response was set to the Answers) are stored to the dirtyList.
 */
class AnswerQueue {
	friend Answer;
public:
	AnswerQueue();

	/*
	 * Blocking waiting for the list of the Answers in which
	 * change Results were.  Returns true if the event became
	 * signalled within the specified time interval, false otherwise.
	 */
	bool wait(const Poco::Timespan &timeout,
		std::list<Answer::Ptr> &dirtyList);

	void remove(const Answer::Ptr answer);

	Poco::Event &event();

	unsigned long size() const;

protected:
	void add(Answer *answer);

	bool block(const Poco::Timespan &timeout);

	/*
	 * List of Answer, which were set as dirty.
	 */
	void listDirty(std::list<Answer::Ptr> &dirtyList) const;

protected:
	std::list<Answer::Ptr> m_answerList;
	Poco::Event m_event;
	mutable Poco::FastMutex m_mutex;
};

}

#endif
