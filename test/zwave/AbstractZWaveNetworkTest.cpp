#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Clock.h>

#include "cppunit/BetterAssert.h"
#include "zwave/AbstractZWaveNetwork.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

using PollEvent = ZWaveNetwork::PollEvent;

namespace BeeeOn {

class AbstractZWaveNetworkTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(AbstractZWaveNetworkTest);
	CPPUNIT_TEST(testPollTimeout);
	CPPUNIT_TEST_SUITE_END();

public:
	void testPollTimeout();
};

CPPUNIT_TEST_SUITE_REGISTRATION(AbstractZWaveNetworkTest);

class TestableAbstractZWaveNetwork : public AbstractZWaveNetwork {
public:
	void startInclusion() override
	{
		notifyEvent(PollEvent::createInclusionStart());
	}

	void cancelInclusion() override
	{
		notifyEvent(PollEvent::createInclusionDone());
	}

	void startRemoveNode() override
	{
		notifyEvent(PollEvent::createRemoveNodeStart());
	}

	void cancelRemoveNode() override
	{
		notifyEvent(PollEvent::createRemoveNodeDone());
	}
};

void AbstractZWaveNetworkTest::testPollTimeout()
{
	TestableAbstractZWaveNetwork network;

	const Clock started;

	PollEvent e = network.pollEvent(10 * Timespan::MILLISECONDS);
	CPPUNIT_ASSERT_EQUAL(PollEvent::EVENT_NONE, e.type());
	CPPUNIT_ASSERT(started.elapsed() >= 10 * Timespan::MILLISECONDS);
}

}
