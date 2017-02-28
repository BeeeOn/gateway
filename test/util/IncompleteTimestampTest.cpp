#include <Poco/Timespan.h>

#include <cppunit/extensions/HelperMacros.h>

#include "util/IncompleteTimestamp.h"

using namespace std;
using namespace Poco;

namespace BeeeOn {

class IncompleteTimestampTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(IncompleteTimestampTest);
	CPPUNIT_TEST(testIsComplete);
	CPPUNIT_TEST(testEqual);
	CPPUNIT_TEST(testCompare);
	CPPUNIT_TEST(deriveComplete);
	CPPUNIT_TEST(deriveCompleteFromComplete);
	CPPUNIT_TEST_SUITE_END();
public:
	void testIsComplete();
	void testEqual();
	void testCompare();
	void deriveComplete();
	void deriveCompleteFromComplete();
};

CPPUNIT_TEST_SUITE_REGISTRATION(IncompleteTimestampTest);

void IncompleteTimestampTest::testIsComplete()
{
	IncompleteTimestamp ts; // now

	// complete
	CPPUNIT_ASSERT(ts.isComplete());

	ts = Timestamp::fromEpochTime(TimestampCompleteTest::TOO_OLD + 1);

	CPPUNIT_ASSERT(ts.isComplete());

	// incomplete
	ts = Timestamp(TimestampCompleteTest::TOO_OLD);

	CPPUNIT_ASSERT(!ts.isComplete());

	ts = Timestamp(0);
	CPPUNIT_ASSERT(!ts.isComplete());
}

void IncompleteTimestampTest::testEqual()
{
	Timestamp now;

	IncompleteTimestamp ts0(now);
	IncompleteTimestamp ts1(now);
	IncompleteTimestamp ts2(now + 1);

	CPPUNIT_ASSERT(ts0 == ts0);
	CPPUNIT_ASSERT(ts0 == ts1);
	CPPUNIT_ASSERT(ts0 != ts2);

	IncompleteTimestamp incomplete0;
	IncompleteTimestamp incomplete1;
	IncompleteTimestamp incomplete2;
	incomplete0 = Timestamp::fromEpochTime(TimestampCompleteTest::TOO_OLD);
	incomplete1 = Timestamp::fromEpochTime(TimestampCompleteTest::TOO_OLD);
	incomplete2 = Timestamp::fromEpochTime(
				       TimestampCompleteTest::TOO_OLD - 1);

	CPPUNIT_ASSERT(incomplete0 == incomplete0);
	CPPUNIT_ASSERT(incomplete0 == incomplete1);
	CPPUNIT_ASSERT(incomplete0 != incomplete2);
}

void IncompleteTimestampTest::testCompare()
{
	Timestamp now;

	IncompleteTimestamp ts0(now);
	IncompleteTimestamp ts1(now);
	IncompleteTimestamp ts2(now - 1);
	IncompleteTimestamp ts3(
		Timestamp::fromEpochTime(TimestampCompleteTest::TOO_OLD));
	IncompleteTimestamp ts4(
		Timestamp::fromEpochTime(TimestampCompleteTest::TOO_OLD));
	IncompleteTimestamp ts5(
		Timestamp::fromEpochTime(TimestampCompleteTest::TOO_OLD - 1));

	CPPUNIT_ASSERT(ts2 < ts0);
	CPPUNIT_ASSERT(!(ts2 > ts0));

	CPPUNIT_ASSERT(!(ts0 < ts1)); // they equal
	CPPUNIT_ASSERT(!(ts1 > ts0)); // they equal

	CPPUNIT_ASSERT(ts3 < ts0);
	CPPUNIT_ASSERT(!(ts3 > ts0));

	CPPUNIT_ASSERT(ts3 < ts2);
	CPPUNIT_ASSERT(!(ts3 > ts2));

	CPPUNIT_ASSERT(!(ts3 < ts4)); // they equal
	CPPUNIT_ASSERT(!(ts4 > ts3)); // they equal

	CPPUNIT_ASSERT(ts5 < ts3);
	CPPUNIT_ASSERT(!(ts3 < ts5));

	CPPUNIT_ASSERT(ts5 < ts0);
	CPPUNIT_ASSERT(!(ts5 > ts0));
}

void IncompleteTimestampTest::deriveComplete()
{
	// 1 hour after 1.1.1970
	IncompleteTimestamp ts(Timestamp(60 * 60 * 1000000UL));
	// uptime 2 hours
	Timespan uptime(2 * 60 * 60, 0);

	Timestamp now;
	Timestamp derived = ts.deriveComplete({uptime, now});

	// derived must be now - 1 hour
	CPPUNIT_ASSERT(now - Timespan(60 * 60, 0) == derived);
}

void IncompleteTimestampTest::deriveCompleteFromComplete()
{
	// now - complete
	const IncompleteTimestamp ts;
	// uptime 1 hour
	Timespan uptime(60 * 60, 0);

	Timestamp now;
	Timestamp derived = ts.deriveComplete({uptime, now});

	// derived must not be different
	CPPUNIT_ASSERT(ts.value() == derived);
}

}
