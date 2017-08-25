#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Event.h>
#include <Poco/SharedPtr.h>
#include <Poco/Thread.h>

#include "cppunit/BetterAssert.h"
#include "core/DongleDeviceManager.h"
#include "hotplug/HotplugEvent.h"

using namespace std;
using namespace Poco;

namespace BeeeOn {

class TestableDongleDeviceManager : public DongleDeviceManager {
public:
	TestableDongleDeviceManager(
			const std::string name, const DevicePrefix &prefix):
		DongleDeviceManager(prefix),
		m_name(name)
	{
	}

	bool accept(const Command::Ptr) override
	{
		return false;
	}

	void handle(Command::Ptr, Answer::Ptr) override
	{
	}

	string dongleMatch(const HotplugEvent &e) override
	{
		if (e.name() == m_name)
			return m_name;

		return "";
	}

	void dongleAvailable() override
	{
		m_becameAvailable.set();
		available().wait();
		done().set();

		// here we would fail if no dongle is available
		(void) dongleName();
	}

	bool dongleMissing() override
	{
		m_becameMissing.set();
		missing().wait();
		done().set();
		return false;
	}

	Event &available()
	{
		return m_available;
	}

	Event &becameAvailable()
	{
		return m_becameAvailable;
	}

	Event &missing()
	{
		return m_missing;
	}

	Event &becameMissing()
	{
		return m_becameMissing;
	}

	Event &done()
	{
		return m_done;
	}

private:
	string m_name;
	Event m_becameAvailable;
	Event m_becameMissing;
	Event m_available;
	Event m_missing;
	Event m_done;
};

class AlwaysFailingDongleDeviceManager : public DongleDeviceManager {
public:
	static const string MATCHING_NAME;

	AlwaysFailingDongleDeviceManager(
			const std::string name,
			const DevicePrefix &prefix,
			std::function<void()> success,
			std::function<void()> fail):
		DongleDeviceManager(prefix),
		m_success(success),
		m_fail(fail),
		m_attempts(0),
		m_name(name)
	{
	}

	bool accept(const Command::Ptr) override
	{
		return false;
	}

	void handle(Command::Ptr, Answer::Ptr) override
	{
	}

	string dongleMatch(const HotplugEvent &e) override
	{
		if (e.name() == m_name)
			return m_name;

		return "";
	}

	void dongleAvailable() override
	{
		m_attempts += 1;

		throw Exception("always failing");
	}

	bool dongleMissing() override
	{
		return true;
	}

	void dongleFailed(const FailDetector &) override
	{
		if (m_attempts == 3)
			m_success();
		else
			m_fail();

		m_attempts = 0;
		event().wait();
	}

private:
	std::function<void()> m_success;
	std::function<void()> m_fail;
	int m_attempts;
	string m_name;
};

const string AlwaysFailingDongleDeviceManager::MATCHING_NAME = "failing";

class DongleDeviceManagerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DongleDeviceManagerTest);
	CPPUNIT_TEST(testNoDongleRun);
	CPPUNIT_TEST(testAddDongleRun);
	CPPUNIT_TEST(testAddDongleBeforeRun);
	CPPUNIT_TEST(testAddRemoveDongleRun);
	CPPUNIT_TEST(testFailDetection);
	CPPUNIT_TEST_SUITE_END();

public:
	DongleDeviceManagerTest();

	void setUp() override;

	void testNoDongleRun();
	void testAddDongleRun();
	void testAddDongleBeforeRun();
	void testAddRemoveDongleRun();
	void testFailDetection();

private:
	SharedPtr<TestableDongleDeviceManager> m_manager;
	SharedPtr<Thread> m_thread;
};

CPPUNIT_TEST_SUITE_REGISTRATION(DongleDeviceManagerTest);

DongleDeviceManagerTest::DongleDeviceManagerTest()
{
}

void DongleDeviceManagerTest::setUp()
{
	m_manager = new TestableDongleDeviceManager("testing-device", DevicePrefix::PREFIX_JABLOTRON);
	m_thread = new Thread;
}

/**
 * Make sure that the DongleDeviceManager only executes the
 * dongleMissing() routine.
 */
void DongleDeviceManagerTest::testNoDongleRun()
{
	m_thread->start(*m_manager);

	CPPUNIT_ASSERT(m_manager->becameMissing().tryWait(10000));
	// we are inside the dongleMissing()
	m_manager->missing().set();
	CPPUNIT_ASSERT(m_manager->done().tryWait(10000));

	CPPUNIT_ASSERT(m_manager->becameMissing().tryWait(10000));
	// we are inside the dongleMissing()
	m_manager->stop();

	m_manager->missing().set();
	CPPUNIT_ASSERT(m_manager->done().tryWait(10000));

	// we are stopped and leaving the main loop
	CPPUNIT_ASSERT(m_thread->tryJoin(10000));
}

/**
 * Make sure that the DongleDeviceManager executes the
 * dongleMissing() routine and then after a dongle is made
 * available the dongleAvailable() is executed.
 *
 * The TestableDongleDeviceManager must gracefully finish
 * because the dongleAvailable() method just returns.
 */
void DongleDeviceManagerTest::testAddDongleRun()
{
	HotplugEvent event;

	m_thread->start(*m_manager);

	CPPUNIT_ASSERT(m_manager->becameMissing().tryWait(10000));
	// we are inside the dongleMissing()
	m_manager->missing().set();
	CPPUNIT_ASSERT(m_manager->done().tryWait(10000));

	CPPUNIT_ASSERT(m_manager->becameMissing().tryWait(10000));
	// we are inside the dongleMissing()

	event.setName("testing-device");
	m_manager->onAdd(event);

	// wakeup from dongleMissing and check for dongle
	m_manager->missing().set();
	CPPUNIT_ASSERT(m_manager->done().tryWait(10000));

	CPPUNIT_ASSERT(m_manager->becameAvailable().tryWait(10000));
	// we are inside the dongleAvailable()
	m_manager->available().set();
	CPPUNIT_ASSERT(m_manager->done().tryWait(10000));

	// dongleAvailable() has finished gracefully
	CPPUNIT_ASSERT(m_thread->tryJoin(10000));
}

/**
 * Make sure that the DongleDeviceManager executes only the
 * dongleAvailable() because a dongle is already add before
 * the run() starts.
 *
 * The TestableDongleDeviceManager must gracefully finish
 * because the dongleAvailable() method just returns.
 */
void DongleDeviceManagerTest::testAddDongleBeforeRun()
{
	HotplugEvent event;

	event.setName("testing-device");
	m_manager->onAdd(event);

	m_thread->start(*m_manager);

	CPPUNIT_ASSERT(m_manager->becameAvailable().tryWait(10000));
	// we are inside the dongleAvailable()
	m_manager->available().set();
	CPPUNIT_ASSERT(m_manager->done().tryWait(10000));

	// dongleAvailable() has finished gracefully
	CPPUNIT_ASSERT(m_thread->tryJoin(10000));
}

/**
 * Make sure that the DongleDeviceManager executes the
 * dongleMissing() routine then again while adding a new
 * dongle. When the dongle is added the dongleAvailable()
 * is executed. During the dongleAvailable(), the dongle
 * is removed and thus we should be back in the dongleMissing()
 * loop where we stop the TestableDongleDeviceManager.
 */
void DongleDeviceManagerTest::testAddRemoveDongleRun()
{
	HotplugEvent event;
	event.setName("testing-device");

	m_thread->start(*m_manager);

	CPPUNIT_ASSERT(m_manager->becameMissing().tryWait(10000));
	// we are inside the dongleMissing()
	m_manager->missing().set();
	CPPUNIT_ASSERT(m_manager->done().tryWait(10000));

	CPPUNIT_ASSERT(m_manager->becameMissing().tryWait(10000));
	// we are inside the dongleMissing()

	m_manager->onAdd(event);

	m_manager->missing().set();
	CPPUNIT_ASSERT(m_manager->done().tryWait(10000));

	CPPUNIT_ASSERT(m_manager->becameAvailable().tryWait(10000));
	// no we must be inside dongleAvailable()
	m_manager->onRemove(event);

	m_manager->available().set();
	CPPUNIT_ASSERT(m_manager->done().tryWait(10000));

	CPPUNIT_ASSERT(m_manager->becameMissing().tryWait(10000));
	// we are again inside the dongleMissing()
	m_manager->missing().set();
	CPPUNIT_ASSERT(m_manager->done().tryWait(10000));

	CPPUNIT_ASSERT(m_manager->becameMissing().tryWait(10000));
	// we are inside the dongleMissing()
	m_manager->stop();

	m_manager->missing().set();
	CPPUNIT_ASSERT(m_manager->done().tryWait(10000));

	CPPUNIT_ASSERT(m_thread->tryJoin(10000));
}

void DongleDeviceManagerTest::testFailDetection()
{
	enum {
		R_NONE,
		R_SUCCESS,
		R_FAIL
	} result = R_NONE;

	Event sync;

	AlwaysFailingDongleDeviceManager manager(
		AlwaysFailingDongleDeviceManager::MATCHING_NAME,
		DevicePrefix::PREFIX_JABLOTRON,
		[&]() {result = R_SUCCESS; sync.set();},
		[&]() {result = R_FAIL;    sync.set();}
	);

	m_thread->start(manager);

	HotplugEvent event;
	event.setName(AlwaysFailingDongleDeviceManager::MATCHING_NAME);

	manager.onAdd(event);
	CPPUNIT_ASSERT(sync.tryWait(10000));
	CPPUNIT_ASSERT_EQUAL(R_SUCCESS, result);

	manager.stop();
	CPPUNIT_ASSERT(m_thread->tryJoin(10000));
}

}
