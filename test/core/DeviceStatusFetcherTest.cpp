#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Event.h>
#include <Poco/Thread.h>

#include "commands/ServerDeviceListCommand.h"
#include "commands/ServerDeviceListResult.h"
#include "core/CommandDispatcher.h"
#include "core/DeviceStatusFetcher.h"
#include "cppunit/BetterAssert.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class DeviceStatusFetcherTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DeviceStatusFetcherTest);
	CPPUNIT_TEST(testSingleHandler);
	CPPUNIT_TEST(testMultipleHandlers);
	CPPUNIT_TEST(testNoDevicesForHandlers);
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();

	void testSingleHandler();
	void testMultipleHandlers();
	void testNoDevicesForHandlers();

private:
	DeviceStatusFetcher::Ptr m_fetcher;
};

CPPUNIT_TEST_SUITE_REGISTRATION(DeviceStatusFetcherTest);

class TestingDeviceStatusHandler : public DeviceStatusHandler {
public:
	typedef SharedPtr<TestingDeviceStatusHandler> Ptr;

	TestingDeviceStatusHandler(const DevicePrefix &prefix):
		m_prefix(prefix)
	{
	}

	DevicePrefix prefix() const override
	{
		return m_prefix;
	}

	void handleRemoteStatus(
		const DevicePrefix &prefix,
		const set<DeviceID> &paired,
		const DeviceStatusHandler::DeviceValues &values)
	{
		handledPrefix = prefix;
		handledPaired = paired;
		handledValues = values;

		handled.set();
	}

	Event handled;
	DevicePrefix handledPrefix = DevicePrefix::PREFIX_INVALID;
	set<DeviceID> handledPaired;
	DeviceStatusHandler::DeviceValues handledValues;

private:
	DevicePrefix m_prefix;
};

class TestingCommandDispatcherForFetcher : public CommandDispatcher {
public:
	typedef SharedPtr<TestingCommandDispatcherForFetcher> Ptr;

protected:
	void dispatchImpl(Command::Ptr cmd, Answer::Ptr answer) override
	{
		ServerDeviceListCommand::Ptr request = cmd.cast<ServerDeviceListCommand>();

		answer->setHandlersCount(1);
		const auto devices = this->devices;

		m_thread.startFunc([request, answer, devices]() mutable
		{
			ServerDeviceListResult::Ptr result =
				new ServerDeviceListResult(answer);
			vector<DeviceID> list;

			for (const auto &id : devices) {
				if (id.prefix() != request->devicePrefix())
					continue;

				list.emplace_back(id);
			}

			result->setDeviceList(list);
			result->setStatus(Result::Status::SUCCESS);
			answer->addResult(result);
		});

		m_thread.join();
	}

public:
	set<DeviceID> devices;
	Thread m_thread;
};

void DeviceStatusFetcherTest::setUp()
{
	m_fetcher = new DeviceStatusFetcher;
	m_fetcher->setIdleDuration(2 * Timespan::SECONDS);
	m_fetcher->setWaitTimeout(20 * Timespan::MILLISECONDS);
	m_fetcher->setRepeatTimeout(200 * Timespan::MILLISECONDS);
}

/**
 * @brief Test that the DeviceStatusFetcher handles a single registered
 * DeviceStatusHandler properly.
 */
void DeviceStatusFetcherTest::testSingleHandler()
{
	Thread thread;
	TestingDeviceStatusHandler::Ptr handler =
		new TestingDeviceStatusHandler(DevicePrefix::PREFIX_VIRTUAL_DEVICE);
	TestingCommandDispatcherForFetcher::Ptr dispatcher =
		new TestingCommandDispatcherForFetcher;

	m_fetcher->registerHandler(handler);
	m_fetcher->setCommandDispatcher(dispatcher);

	dispatcher->devices = {
		0xa300000000000001,
		0xa300000000000002,
	};

	thread.start(*m_fetcher);

	CPPUNIT_ASSERT(handler->handled.tryWait(10000));

	CPPUNIT_ASSERT(handler->handledPrefix == DevicePrefix::PREFIX_VIRTUAL_DEVICE);
	CPPUNIT_ASSERT_EQUAL(2, handler->handledPaired.size());

	m_fetcher->stop();
	CPPUNIT_ASSERT_NO_THROW(thread.join(10000));
}

/**
 * @brief Test that the DeviceStatusFetcher handles a single registered
 * DeviceStatusHandler properly.
 */
void DeviceStatusFetcherTest::testMultipleHandlers()
{
	Thread thread;
	TestingDeviceStatusHandler::Ptr handler0 =
		new TestingDeviceStatusHandler(DevicePrefix::PREFIX_VIRTUAL_DEVICE);
	TestingDeviceStatusHandler::Ptr handler1 =
		new TestingDeviceStatusHandler(DevicePrefix::PREFIX_PRESSURE_SENSOR);
	TestingDeviceStatusHandler::Ptr handler2 =
		new TestingDeviceStatusHandler(DevicePrefix::PREFIX_FITPROTOCOL);
	TestingDeviceStatusHandler::Ptr handler3 =
		new TestingDeviceStatusHandler(DevicePrefix::PREFIX_BLUETOOTH);

	TestingCommandDispatcherForFetcher::Ptr dispatcher =
		new TestingCommandDispatcherForFetcher;

	m_fetcher->registerHandler(handler0);
	m_fetcher->registerHandler(handler1);
	m_fetcher->registerHandler(handler2);
	m_fetcher->registerHandler(handler3);
	m_fetcher->setCommandDispatcher(dispatcher);

	dispatcher->devices = {
		0xa100000000000001, // fitp
		0xa100000000000002, // fitp
		0xa200000000000003, // pressure
		0xa300000000000004, // vdev
		0xa600000000000005, // bluetooth
		0xa600000000000006, // bluetooth
		0xa600000000000007, // bluetooth
	};

	thread.start(*m_fetcher);

	CPPUNIT_ASSERT(handler0->handled.tryWait(5000));
	CPPUNIT_ASSERT(handler0->handledPrefix == DevicePrefix::PREFIX_VIRTUAL_DEVICE);
	CPPUNIT_ASSERT_EQUAL(1, handler0->handledPaired.size());

	CPPUNIT_ASSERT(handler1->handled.tryWait(5000));
	CPPUNIT_ASSERT(handler1->handledPrefix == DevicePrefix::PREFIX_PRESSURE_SENSOR);
	CPPUNIT_ASSERT_EQUAL(1, handler1->handledPaired.size());

	CPPUNIT_ASSERT(handler2->handled.tryWait(5000));
	CPPUNIT_ASSERT(handler2->handledPrefix == DevicePrefix::PREFIX_FITPROTOCOL);
	CPPUNIT_ASSERT_EQUAL(2, handler2->handledPaired.size());

	CPPUNIT_ASSERT(handler3->handled.tryWait(5000));
	CPPUNIT_ASSERT(handler3->handledPrefix == DevicePrefix::PREFIX_BLUETOOTH);
	CPPUNIT_ASSERT_EQUAL(3, handler3->handledPaired.size());

	m_fetcher->stop();
	CPPUNIT_ASSERT_NO_THROW(thread.join(10000));
}

/**
 * @brief Test that empty set of devices is received when no devices of the
 * expected prefix are received.
 */
void DeviceStatusFetcherTest::testNoDevicesForHandlers()
{
	Thread thread;
	TestingDeviceStatusHandler::Ptr handler0 =
		new TestingDeviceStatusHandler(DevicePrefix::PREFIX_VIRTUAL_DEVICE);
	TestingDeviceStatusHandler::Ptr handler1 =
		new TestingDeviceStatusHandler(DevicePrefix::PREFIX_FITPROTOCOL);
	TestingDeviceStatusHandler::Ptr handler2 =
		new TestingDeviceStatusHandler(DevicePrefix::PREFIX_BLUETOOTH);
	TestingCommandDispatcherForFetcher::Ptr dispatcher =
		new TestingCommandDispatcherForFetcher;

	m_fetcher->registerHandler(handler0);
	m_fetcher->registerHandler(handler1);
	m_fetcher->registerHandler(handler2);
	m_fetcher->setCommandDispatcher(dispatcher);

	dispatcher->devices = {
		0xa600000000000001,
		0xa600000000000002,
		0xa600000000000003,
		0xa600000000000004,
	};

	thread.start(*m_fetcher);

	CPPUNIT_ASSERT(handler0->handled.tryWait(5000));
	CPPUNIT_ASSERT(handler0->handledPrefix == DevicePrefix::PREFIX_VIRTUAL_DEVICE);
	CPPUNIT_ASSERT_EQUAL(0, handler0->handledPaired.size());

	CPPUNIT_ASSERT(handler1->handled.tryWait(5000));
	CPPUNIT_ASSERT(handler1->handledPrefix == DevicePrefix::PREFIX_FITPROTOCOL);
	CPPUNIT_ASSERT_EQUAL(0, handler1->handledPaired.size());

	CPPUNIT_ASSERT(handler2->handled.tryWait(5000));
	CPPUNIT_ASSERT(handler2->handledPrefix == DevicePrefix::PREFIX_BLUETOOTH);
	CPPUNIT_ASSERT_EQUAL(4, handler2->handledPaired.size());

	m_fetcher->stop();
	CPPUNIT_ASSERT_NO_THROW(thread.join(10000));
}

}
