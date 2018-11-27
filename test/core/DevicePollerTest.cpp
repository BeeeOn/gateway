#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Exception.h>

#include "cppunit/BetterAssert.h"
#include "core/DevicePoller.h"
#include "util/NonAsyncExecutor.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class DevicePollerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DevicePollerTest);
	CPPUNIT_TEST(testGrabRefresh);
	CPPUNIT_TEST(testDontScheduleNonSchedulable);
	CPPUNIT_TEST(testDoPoll);
	CPPUNIT_TEST(testPollNextIfOnSchedule);
	CPPUNIT_TEST(testDontRescheduleInactive);
	CPPUNIT_TEST(testRescheduleAfterPoll);
	CPPUNIT_TEST(testCancel);
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
	void testGrabRefresh();
	void testDontScheduleNonSchedulable();
	void testDoPoll();
	void testPollNextIfOnSchedule();
	void testDontRescheduleInactive();
	void testRescheduleAfterPoll();
	void testCancel();

private:
	NonAsyncExecutor::Ptr m_executor;
};

CPPUNIT_TEST_SUITE_REGISTRATION(DevicePollerTest);

class TestableDevicePoller : public DevicePoller {
public:
	using DevicePoller::grabRefresh;
	using DevicePoller::reschedule;
	using DevicePoller::doSchedule;
	using DevicePoller::pollNextIfOnSchedule;
	using DevicePoller::doPoll;
};

class TestingPollableDevice : public PollableDevice {
public:
	typedef Poco::SharedPtr<TestingPollableDevice> Ptr;

	TestingPollableDevice(
			const DeviceID &id,
			const RefreshTime &refresh):
		m_polled(0),
		m_failNextPoll(false),
		m_id(id),
		m_refresh(refresh)
	{
	}

	DeviceID id() const override
	{
		return m_id;
	}

	RefreshTime refresh() const override
	{
		return m_refresh;
	}

	void failNextPoll()
	{
		m_failNextPoll = true;
	}

	void poll(Distributor::Ptr) override
	{
		if (m_failNextPoll) {
			throw IOException("polling intentionally failed");
			m_failNextPoll = false;
		}

		m_polled += 1;
	}

	size_t polled() const
	{
		return m_polled;
	}

private:
	size_t m_polled;
	bool m_failNextPoll;
	DeviceID m_id;
	RefreshTime m_refresh;
};

void DevicePollerTest::setUp()
{
	m_executor = new NonAsyncExecutor;
}

/**
 * @brief Test that grabRefresh() fails on unusable refresh times
 * and extracts the time from usable refresh times.
 */
void DevicePollerTest::testGrabRefresh()
{
	TestingPollableDevice::Ptr valid = new TestingPollableDevice(
			DeviceID::random(), RefreshTime::fromSeconds(5));
	TestingPollableDevice::Ptr none = new TestingPollableDevice(
			DeviceID::random(), RefreshTime::NONE);
	TestingPollableDevice::Ptr disabled = new TestingPollableDevice(
			DeviceID::random(), RefreshTime::DISABLED);

	CPPUNIT_ASSERT_EQUAL(
		5 * Timespan::SECONDS,
		TestableDevicePoller::grabRefresh(valid).totalMicroseconds());

	CPPUNIT_ASSERT_THROW(
		TestableDevicePoller::grabRefresh(none),
		IllegalStateException);

	CPPUNIT_ASSERT_THROW(
		TestableDevicePoller::grabRefresh(disabled),
		IllegalStateException);
}

/**
 * @brief Test that only schedulable devices (with valid refresh time)
 * are scheduled. For other, the IllegalStateException is thrown.
 */
void DevicePollerTest::testDontScheduleNonSchedulable()
{
	TestingPollableDevice::Ptr valid = new TestingPollableDevice(
			DeviceID::random(), RefreshTime::fromSeconds(5));
	TestingPollableDevice::Ptr none = new TestingPollableDevice(
			DeviceID::random(), RefreshTime::NONE);
	TestingPollableDevice::Ptr disabled = new TestingPollableDevice(
			DeviceID::random(), RefreshTime::DISABLED);

	TestableDevicePoller poller;
	poller.setPollExecutor(m_executor);

	CPPUNIT_ASSERT_NO_THROW(poller.doSchedule(valid, 0));
	CPPUNIT_ASSERT_THROW(
		poller.doSchedule(none, 0),
		IllegalStateException);
	CPPUNIT_ASSERT_THROW(
		poller.doSchedule(disabled, 0),
		IllegalStateException);
}

/**
 * @brief Test that the given device is polled by call doPoll()
 * via a configured executor. Exceptions are caught nad thus the
 * doPoll() itself should not fail.
 */
void DevicePollerTest::testDoPoll()
{
	TestableDevicePoller poller;
	poller.setPollExecutor(m_executor);

	TestingPollableDevice::Ptr device = new TestingPollableDevice(
			DeviceID::random(), RefreshTime::fromSeconds(5));

	CPPUNIT_ASSERT_EQUAL(0, device->polled());

	poller.doPoll(device);

	CPPUNIT_ASSERT_EQUAL(1, device->polled());

	device->failNextPoll();
	poller.doPoll(device); // exception inside of executor

	CPPUNIT_ASSERT_EQUAL(1, device->polled());
}

/**
 * @brief Test some computed sleeping delays from pollNextIfOnSchedule()
 * up to the point when a device is asked to poll.
 */
void DevicePollerTest::testPollNextIfOnSchedule()
{
	TestableDevicePoller poller;
	poller.setPollExecutor(m_executor);

	TestingPollableDevice::Ptr device = new TestingPollableDevice(
			DeviceID::random(), RefreshTime::fromSeconds(5));

	CPPUNIT_ASSERT_EQUAL(0, device->polled());

	// polling next when empty fails on assert
	CPPUNIT_ASSERT_THROW(
		poller.pollNextIfOnSchedule(0),
		AssertionViolationException);

	poller.doSchedule(device, 0);

	for (int i = 0; i <= device->refresh().seconds(); ++i) {
		CPPUNIT_ASSERT_EQUAL_MESSAGE(
			"at " + to_string(i) + " second",
			0, device->polled());

		// check sleeping delay as time passes
		CPPUNIT_ASSERT_EQUAL_MESSAGE(
			"at " + to_string(i) + " second",
			(5 - i) * Timespan::SECONDS,
			poller.pollNextIfOnSchedule(i * Timespan::SECONDS)
				.totalMicroseconds());
	}

	CPPUNIT_ASSERT_EQUAL(1, device->polled());
}

/**
 * @brief The method reschedule() can reschedule only device that is
 * currently marked active. If it is not active, nothing happens.
 */
void DevicePollerTest::testDontRescheduleInactive()
{
	TestableDevicePoller poller;
	poller.setPollExecutor(m_executor);

	TestingPollableDevice::Ptr device = new TestingPollableDevice(
			DeviceID::random(), RefreshTime::fromSeconds(5));

	poller.reschedule(device, 0);

	// there must still be nothing to poll
	CPPUNIT_ASSERT_THROW(
		poller.pollNextIfOnSchedule(0),
		AssertionViolationException);
}

/**
 * @brief Check that a device is rescheduled after poll.
 */
void DevicePollerTest::testRescheduleAfterPoll()
{
	TestableDevicePoller poller;
	poller.setPollExecutor(m_executor);

	TestingPollableDevice::Ptr device = new TestingPollableDevice(
			DeviceID::random(), RefreshTime::fromSeconds(5));

	const Clock before;

	// schedule device
	poller.doSchedule(device, 0);

	// activate, poll and reschedule device
	CPPUNIT_ASSERT_EQUAL(
		0,
		poller.pollNextIfOnSchedule(5 * Timespan::SECONDS)
			.totalMicroseconds());

	// rescheduled device would be actived based on current Clock
	CPPUNIT_ASSERT(poller.pollNextIfOnSchedule(before) >= 0);
}

/**
 * @brief Check that cancel prevents a devices from being polled.
 */
void DevicePollerTest::testCancel()
{
	TestableDevicePoller poller;
	poller.setPollExecutor(m_executor);

	TestingPollableDevice::Ptr device = new TestingPollableDevice(
			DeviceID::random(), RefreshTime::fromSeconds(5));

	CPPUNIT_ASSERT_NO_THROW(poller.cancel(device->id()));

	poller.doSchedule(device, 0);

	// it would be polled in 5 seconds
	CPPUNIT_ASSERT_EQUAL(
		5 * Timespan::SECONDS,
		poller.pollNextIfOnSchedule(0).totalMicroseconds());

	poller.cancel(device->id());

	// there must still be nothing to poll after cancel
	CPPUNIT_ASSERT_THROW(
		poller.pollNextIfOnSchedule(0),
		AssertionViolationException);
}

}
