#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Timer.h>
#include <Poco/Timespan.h>

#include "cppunit/BetterAssert.h"

#include "core/AnswerQueue.h"

using namespace Poco;

namespace BeeeOn {

class AnswerQueueTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(AnswerQueueTest);
	CPPUNIT_TEST(testListDirty);
	CPPUNIT_TEST(testWaitTimeout);
	CPPUNIT_TEST(testRemove);
	CPPUNIT_TEST(testResultUpdated);
	CPPUNIT_TEST_SUITE_END();

public:
	void testListDirty();
	void testWaitTimeout();
	void testRemove();
	void testResultUpdated();
};

CPPUNIT_TEST_SUITE_REGISTRATION(AnswerQueueTest);

class TestableAnswerQueue : public AnswerQueue {
public:
	using AnswerQueue::listDirty;
	using AnswerQueue::m_answerList;
};

class DoLater {
public:
	DoLater(AnswerQueue &queue):
		m_queue(queue),
		m_callback(*this, &DoLater::fire)
	{
	}

	void start(Timer &timer)
	{
		timer.start(m_callback);
}

protected:
	void fire(Timer &)
	{
		fireImpl();
	}

	virtual void fireImpl() = 0;

protected:
	AnswerQueue &m_queue;

private:
	Poco::TimerCallback<DoLater> m_callback;
};

class ResultSetStatusLater : public DoLater {
public:
	ResultSetStatusLater(AnswerQueue &queue, Result::Ptr result,
			Result::Status status):
		DoLater(queue),
		m_result(result),
		m_status(status)
	{
	}

	void fireImpl() override
	{
		m_result->setStatus(m_status);
	}

private:
	Result::Ptr m_result;
	Result::Status m_status;
};

class AnswerSetDirtyTrueLater : public DoLater {
public:
	AnswerSetDirtyTrueLater(AnswerQueue &queue, Answer::Ptr answer,
			bool dirty):
		DoLater(queue),
		m_dirty(dirty),
		m_answer(answer)
	{
	}

	void fireImpl() override
	{
		m_answer->setDirty(m_dirty);
		m_answer->event().set();
	}

private:
	bool m_dirty;
	Answer::Ptr m_answer;
};

/*
 * Test whether listDirty handles the isDirty() properly. Non-dirty
 * answer is never returned.
 */
void AnswerQueueTest::testListDirty()
{
	TestableAnswerQueue queue;
	Answer::Ptr answer = new Answer(queue);

	std::list<Answer::Ptr> answerList;

	queue.listDirty(answerList);
	CPPUNIT_ASSERT(answerList.empty());

	answer->setDirty(true);
	queue.listDirty(answerList);
	CPPUNIT_ASSERT(1 == answerList.size());
	CPPUNIT_ASSERT_EQUAL(answer, answerList.front());

	answerList.clear();
	answer->setDirty(false);
	queue.listDirty(answerList);
	CPPUNIT_ASSERT(answerList.empty());
}

/*
 * Test wait when timeout is used.
 *
 * 1. wait(0, list) - waiting with zero timeout, timeout expired
 * 2. wait(-1, list) - waiting with negative timeout,
 * 3. wait(1000, list) - waiting with positive timeout, timeout expired
 * 4. wait(100000, list) - waiting with positive timeout, dirty is
 *                         changed in Answer
 * 5. wait(100000, list) - repeats waiting for each new change
 */
void AnswerQueueTest::testWaitTimeout()
{
	TestableAnswerQueue queue;
	Poco::Timestamp now;
	std::list<Answer::Ptr> answerList;
	Answer::Ptr answer = new Answer(queue);

	Poco::Timer deferAfter20(21, 0);
	AnswerSetDirtyTrueLater answerSetDirtyTrueLater(queue, answer, true);

	// waiting with zero timeout, timeout expired
	now.update();
	CPPUNIT_ASSERT(!queue.wait(0, answerList));
	CPPUNIT_ASSERT(answerList.empty());
	CPPUNIT_ASSERT(now.elapsed() >= 0);

	// waiting with negative timeout, timeout expired
	now.update();
	answerSetDirtyTrueLater.start(deferAfter20);
	CPPUNIT_ASSERT(queue.wait(-1, answerList));
	CPPUNIT_ASSERT(1 == answerList.size());
	CPPUNIT_ASSERT(now.elapsed() >= 0);

	// waiting with positive timeout, timeout expired
	now.update();
	answerList.clear();
	CPPUNIT_ASSERT(!queue.wait(1000, answerList));
	CPPUNIT_ASSERT(answerList.empty());
	CPPUNIT_ASSERT(now.elapsed() >= 1000);

	// waiting with positive timeout, dirty is changed in Answer
	answer->setDirty(true);
	now.update();
	CPPUNIT_ASSERT(queue.wait(100000, answerList));
	CPPUNIT_ASSERT(1 == answerList.size());
	CPPUNIT_ASSERT_EQUAL(answer, answerList.front());
	CPPUNIT_ASSERT(now.elapsed() < 100000);

	// repeats waiting for each new change
	for (int i = 0; i < 5; ++i) {
		answerList.clear();
		answer->setDirty(true);
		now.update();

		CPPUNIT_ASSERT(queue.wait(100000, answerList));
		CPPUNIT_ASSERT(1 == answerList.size());
		CPPUNIT_ASSERT_EQUAL(answer, answerList.front());
		CPPUNIT_ASSERT(now.elapsed() < 100000);
	}
}

/*
 * Tests the removal of 3 Answers from AnswerQueue.
 */
void AnswerQueueTest::testRemove()
{
	TestableAnswerQueue queue;
	std::list<Answer::Ptr> answerList;

	Answer::Ptr answer0 = new Answer(queue);
	Answer::Ptr answer1 = new Answer(queue);
	Answer::Ptr answer2 = new Answer(queue);

	answer0->setDirty(true);
	answer1->setDirty(true);
	answer2->setDirty(true);

	answerList.clear();
	queue.listDirty(answerList);
	CPPUNIT_ASSERT(3 == answerList.size());

	// dirtyList is empty
	answerList.clear();
	queue.listDirty(answerList);
	CPPUNIT_ASSERT(answerList.empty());

	queue.remove(answer2);
	CPPUNIT_ASSERT(2 == queue.m_answerList.size());

	// answer2 already removed
	queue.remove(answer2);
	CPPUNIT_ASSERT(2 == queue.m_answerList.size());

	queue.remove(answer1);
	CPPUNIT_ASSERT(1 == queue.m_answerList.size());

	queue.remove(answer0);
	CPPUNIT_ASSERT(0 == queue.m_answerList.size());
}

/*
 * Verify that the queue is notified after the Answer is marked as dirty.
 */
void AnswerQueueTest::testResultUpdated()
{
	TestableAnswerQueue queue;
	std::list<Answer::Ptr> answerList;

	Answer::Ptr answer0 = new Answer(queue);
	Answer::Ptr answer1 = new Answer(queue);

	Result::Ptr result0 = new Result(answer0);

	queue.listDirty(answerList);
	CPPUNIT_ASSERT(answerList.empty());

	Poco::Timer deferAfter20(21, 0);
	ResultSetStatusLater failResult0(queue, result0, Result::FAILED);

	// the result0 must be PENDING here because it has just been created
	CPPUNIT_ASSERT(result0->status() == Result::PENDING);

	Timestamp now;
	failResult0.start(deferAfter20);
	CPPUNIT_ASSERT(queue.wait(100000, answerList));

	// failResult0 wakes the queue up after 20 ms
	CPPUNIT_ASSERT(now.elapsed() >= 20000);
	CPPUNIT_ASSERT(now.elapsed() < 100000);
	CPPUNIT_ASSERT(result0->status() == Result::FAILED);

	CPPUNIT_ASSERT(2 == queue.size());

	queue.remove(answer0);
	CPPUNIT_ASSERT(1 == queue.size());

	queue.remove(answer1);
	CPPUNIT_ASSERT(0 == queue.size());
}

}
