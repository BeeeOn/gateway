#include <queue>

#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"
#include "gwmessage/GWListenRequest.h"
#include "gwmessage/GWResponse.h"
#include "gwmessage/GWSensorDataExport.h"
#include "server/AbstractGWSConnector.h"
#include "server/GWSFixedPriorityAssigner.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class TestableAbstractGWSConnector : public AbstractGWSConnector {
public:
	typedef SharedPtr<TestableAbstractGWSConnector> Ptr;

	using AbstractGWSConnector::selectOutput;
	using AbstractGWSConnector::updateOutputs;
	using AbstractGWSConnector::outputValid;
	using AbstractGWSConnector::peekOutput;
	using AbstractGWSConnector::popOutput;
};

class AbstractGWSConnectorTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(AbstractGWSConnectorTest);
	CPPUNIT_TEST(testSendHighPriority);
	CPPUNIT_TEST(testSendLowPriority);
	CPPUNIT_TEST(testSendMixedPriorities);
	CPPUNIT_TEST(testQueuePrioritiesSimple);
	CPPUNIT_TEST(testQueuePriorities);
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();

	void testSendHighPriority();
	void testSendLowPriority();
	void testSendMixedPriorities();
	void testQueuePrioritiesSimple();
	void testQueuePriorities();

private:
	TestableAbstractGWSConnector::Ptr m_connector;
};

CPPUNIT_TEST_SUITE_REGISTRATION(AbstractGWSConnectorTest);

void AbstractGWSConnectorTest::setUp()
{
	m_connector = new TestableAbstractGWSConnector;
	m_connector->setPriorityAssigner(new GWSFixedPriorityAssigner);
	m_connector->setOutputsCount(4);
	m_connector->setupQueues();
}

static GWMessage::Ptr highPriorityMessage()
{
	GWResponse::Ptr message = new GWResponse;
	message->setID(GlobalID::random());
	return message;
}

static GWMessage::Ptr lowPriorityMessage()
{
	GWSensorDataExport::Ptr message = new GWSensorDataExport;
	message->setID(GlobalID::random());
	return message;
}

static GWMessage::Ptr midPriorityMessage()
{
	GWListenRequest::Ptr message = new GWListenRequest;
	message->setID(GlobalID::random());
	return message;
}

/**
 * @brief Sending high-priority messages only always works.
 */
void AbstractGWSConnectorTest::testSendHighPriority()
{
	CPPUNIT_ASSERT(!m_connector->outputValid(m_connector->selectOutput()));

	m_connector->send(highPriorityMessage());
	CPPUNIT_ASSERT_EQUAL(0, m_connector->selectOutput());

	m_connector->popOutput(0);
	m_connector->updateOutputs(0);
	CPPUNIT_ASSERT(!m_connector->outputValid(m_connector->selectOutput()));

	m_connector->send(highPriorityMessage());
	CPPUNIT_ASSERT_EQUAL(0, m_connector->selectOutput());

	m_connector->send(highPriorityMessage());
	CPPUNIT_ASSERT_EQUAL(0, m_connector->selectOutput());

	m_connector->popOutput(0);
	m_connector->updateOutputs(0);
	CPPUNIT_ASSERT_EQUAL(0, m_connector->selectOutput());

	m_connector->popOutput(0);
	m_connector->updateOutputs(0);
	CPPUNIT_ASSERT(!m_connector->outputValid(m_connector->selectOutput()));
}

/**
 * @brief Sending low-priority messages only always works.
 */
void AbstractGWSConnectorTest::testSendLowPriority()
{
	CPPUNIT_ASSERT(!m_connector->outputValid(m_connector->selectOutput()));

	m_connector->send(lowPriorityMessage());
	CPPUNIT_ASSERT_EQUAL(3, m_connector->selectOutput());

	m_connector->popOutput(3);
	m_connector->updateOutputs(3);
	CPPUNIT_ASSERT(!m_connector->outputValid(m_connector->selectOutput()));

	m_connector->send(lowPriorityMessage());
	CPPUNIT_ASSERT_EQUAL(3, m_connector->selectOutput());

	m_connector->send(lowPriorityMessage());
	CPPUNIT_ASSERT_EQUAL(3, m_connector->selectOutput());

	m_connector->popOutput(3);
	m_connector->updateOutputs(3);
	CPPUNIT_ASSERT_EQUAL(3, m_connector->selectOutput());

	m_connector->popOutput(3);
	m_connector->updateOutputs(3);
	CPPUNIT_ASSERT(!m_connector->outputValid(m_connector->selectOutput()));
}

/**
 * @brief Slow sending of a mix leads to higher-priority first behaviour.
 */
void AbstractGWSConnectorTest::testSendMixedPriorities()
{
	CPPUNIT_ASSERT(!m_connector->outputValid(m_connector->selectOutput()));

	m_connector->send(lowPriorityMessage());
	CPPUNIT_ASSERT_EQUAL(3, m_connector->selectOutput());

	m_connector->send(highPriorityMessage());
	// queue 0 wins over queue 3
	CPPUNIT_ASSERT_EQUAL(0, m_connector->selectOutput());

	m_connector->popOutput(0);
	m_connector->updateOutputs(0);
	CPPUNIT_ASSERT_EQUAL(3, m_connector->selectOutput());

	m_connector->send(midPriorityMessage());
	// queue 1 wins over queue 3
	CPPUNIT_ASSERT_EQUAL(1, m_connector->selectOutput());

	m_connector->popOutput(1);
	m_connector->updateOutputs(1);
	CPPUNIT_ASSERT_EQUAL(3, m_connector->selectOutput());

	m_connector->popOutput(3);
	m_connector->updateOutputs(3);
	CPPUNIT_ASSERT(!m_connector->outputValid(m_connector->selectOutput()));
}

/**
 * @brief If all queues are equivalently filled and has same histories
 * then the higher-priority wins approach is used. The queues are poped
 * in order 0, 1, 3 (queue 2 is unused).
 */
void AbstractGWSConnectorTest::testQueuePrioritiesSimple()
{
	CPPUNIT_ASSERT(!m_connector->outputValid(m_connector->selectOutput()));

	m_connector->send(lowPriorityMessage());
	m_connector->send(midPriorityMessage());
	m_connector->send(highPriorityMessage());

	// 0/1 0/1 0/0 0/1
	CPPUNIT_ASSERT_EQUAL(0, m_connector->selectOutput());

	m_connector->popOutput(0);
	m_connector->updateOutputs(0);
	// 1/0 0/1 0/0 0/1
	CPPUNIT_ASSERT_EQUAL(1, m_connector->selectOutput());

	m_connector->popOutput(1);
	m_connector->updateOutputs(1);
	// 1/0 1/0 0/0 0/1
	CPPUNIT_ASSERT_EQUAL(3, m_connector->selectOutput());

	m_connector->popOutput(3);
	m_connector->updateOutputs(3);
	// 1/0 1/0 0/0 1/0
	CPPUNIT_ASSERT(!m_connector->outputValid(m_connector->selectOutput()));
}

/**
 * @brief If a batch of messages arrives and all queues have the same initial
 * history, we can see that the higher-priority queues are preferred over the
 * lower-priority ones. However, as the queue history is updated, the higher-priority
 * queues are sometimes skipped in favor of the closest lower-priority one.
 */
void AbstractGWSConnectorTest::testQueuePriorities()
{
	CPPUNIT_ASSERT(!m_connector->outputValid(m_connector->selectOutput()));

	m_connector->send(lowPriorityMessage());
	m_connector->send(lowPriorityMessage());
	m_connector->send(lowPriorityMessage());
	m_connector->send(lowPriorityMessage());
	m_connector->send(midPriorityMessage());
	m_connector->send(midPriorityMessage());
	m_connector->send(midPriorityMessage());
	m_connector->send(highPriorityMessage());
	m_connector->send(highPriorityMessage());

	// 0/2 0/3 0/0 0/4 - queue 0 wins
	CPPUNIT_ASSERT_EQUAL(0, m_connector->selectOutput());

	m_connector->popOutput(0);
	m_connector->updateOutputs(0);
	// 1/1 0/3 0/0 0/4 - queue 1 wins over queue 0 because of history
	CPPUNIT_ASSERT_EQUAL(1, m_connector->selectOutput());

	m_connector->popOutput(1);
	m_connector->updateOutputs(1);
	// 1/1 1/2 0/0 0/4 - queue 0 wins over queue 1
	CPPUNIT_ASSERT_EQUAL(0, m_connector->selectOutput());

	m_connector->popOutput(0);
	m_connector->updateOutputs(0);
	// 2/0 1/2 0/0 0/4 - queue 3 wins over queue 1 because of history
	CPPUNIT_ASSERT_EQUAL(3, m_connector->selectOutput());

	m_connector->popOutput(3);
	m_connector->updateOutputs(3);
	// 2/0 1/2 0/0 1/3 - queue 1 wins
	CPPUNIT_ASSERT_EQUAL(1, m_connector->selectOutput());

	m_connector->popOutput(1);
	m_connector->updateOutputs(1);
	// 2/0 2/1 0/0 1/3 - queue 3 wins over queue 1 because of history
	CPPUNIT_ASSERT_EQUAL(3, m_connector->selectOutput());

	m_connector->popOutput(3);
	m_connector->updateOutputs(3);
	// 2/0 2/1 0/0 2/2 - queue 1 wins
	CPPUNIT_ASSERT_EQUAL(1, m_connector->selectOutput());

	m_connector->popOutput(1);
	m_connector->updateOutputs(1);
	// 2/0 3/0 0/0 2/2 - no competition anymore, but...
	CPPUNIT_ASSERT_EQUAL(3, m_connector->selectOutput());

	m_connector->send(midPriorityMessage());
	m_connector->send(highPriorityMessage());
	// 2/1 3/1 0/0 2/2 - queue 0 wins
	CPPUNIT_ASSERT_EQUAL(0, m_connector->selectOutput());

	m_connector->popOutput(0);
	m_connector->updateOutputs(0);
	// 3/0 3/1 0/0 2/2 - queue 3 wins over queue 1 because of history
	CPPUNIT_ASSERT_EQUAL(3, m_connector->selectOutput());

	m_connector->popOutput(3);
	m_connector->updateOutputs(3);
	// 3/0 3/1 0/0 3/1 - queue 1 wins
	CPPUNIT_ASSERT_EQUAL(1, m_connector->selectOutput());

	m_connector->popOutput(1);
	m_connector->updateOutputs(1);
	// 3/0 4/0 0/0 3/1 - no competition anymore
	CPPUNIT_ASSERT_EQUAL(3, m_connector->selectOutput());

	m_connector->popOutput(3);
	m_connector->updateOutputs(3);
	// 3/0 4/0 0/0 4/0
	CPPUNIT_ASSERT(!m_connector->outputValid(m_connector->selectOutput()));
}

}
