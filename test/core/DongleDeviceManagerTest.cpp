#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Event.h>
#include <Poco/Thread.h>

#include "cppunit/BetterAssert.h"
#include "core/DongleDeviceManager.h"
#include "udev/UDevEvent.h"

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

	string dongleMatch(const UDevEvent &e) override
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

class DongleDeviceManagerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DongleDeviceManagerTest);
	CPPUNIT_TEST(testNoDongleRun);
	CPPUNIT_TEST(testAddDongleRun);
	CPPUNIT_TEST(testAddDongleBeforeRun);
	CPPUNIT_TEST(testAddRemoveDongleRun);
	CPPUNIT_TEST_SUITE_END();

public:
	DongleDeviceManagerTest();

	void testNoDongleRun();
	void testAddDongleRun();
	void testAddDongleBeforeRun();
	void testAddRemoveDongleRun();

private:
	TestableDongleDeviceManager m_manager;
	Thread m_thread;
};

CPPUNIT_TEST_SUITE_REGISTRATION(DongleDeviceManagerTest);

DongleDeviceManagerTest::DongleDeviceManagerTest():
	m_manager("testing-device", DevicePrefix::PREFIX_JABLOTRON)
{
}

/**
 * Make sure that the DongleDeviceManager only executes the
 * dongleMissing() routine.
 */
void DongleDeviceManagerTest::testNoDongleRun()
{
	m_thread.start(m_manager);

	CPPUNIT_ASSERT(m_manager.becameMissing().tryWait(10000));
	// we are inside the dongleMissing()
	m_manager.missing().set();
	CPPUNIT_ASSERT(m_manager.done().tryWait(10000));

	CPPUNIT_ASSERT(m_manager.becameMissing().tryWait(10000));
	// we are inside the dongleMissing()
	m_manager.stop();

	m_manager.missing().set();
	CPPUNIT_ASSERT(m_manager.done().tryWait(10000));

	// we are stopped and leaving the main loop
	CPPUNIT_ASSERT(m_thread.tryJoin(10000));
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
	UDevEvent event;

	m_thread.start(m_manager);

	CPPUNIT_ASSERT(m_manager.becameMissing().tryWait(10000));
	// we are inside the dongleMissing()
	m_manager.missing().set();
	CPPUNIT_ASSERT(m_manager.done().tryWait(10000));

	CPPUNIT_ASSERT(m_manager.becameMissing().tryWait(10000));
	// we are inside the dongleMissing()

	event.setName("testing-device");
	m_manager.onAdd(event);

	// wakeup from dongleMissing and check for dongle
	m_manager.missing().set();
	CPPUNIT_ASSERT(m_manager.done().tryWait(10000));

	CPPUNIT_ASSERT(m_manager.becameAvailable().tryWait(10000));
	// we are inside the dongleAvailable()
	m_manager.available().set();
	CPPUNIT_ASSERT(m_manager.done().tryWait(10000));

	// dongleAvailable() has finished gracefully
	CPPUNIT_ASSERT(m_thread.tryJoin(10000));
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
	UDevEvent event;

	event.setName("testing-device");
	m_manager.onAdd(event);

	m_thread.start(m_manager);

	CPPUNIT_ASSERT(m_manager.becameAvailable().tryWait(10000));
	// we are inside the dongleAvailable()
	m_manager.available().set();
	CPPUNIT_ASSERT(m_manager.done().tryWait(10000));

	// dongleAvailable() has finished gracefully
	CPPUNIT_ASSERT(m_thread.tryJoin(10000));
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
	UDevEvent event;
	event.setName("testing-device");

	m_thread.start(m_manager);

	CPPUNIT_ASSERT(m_manager.becameMissing().tryWait(10000));
	// we are inside the dongleMissing()
	m_manager.missing().set();
	CPPUNIT_ASSERT(m_manager.done().tryWait(10000));

	CPPUNIT_ASSERT(m_manager.becameMissing().tryWait(10000));
	// we are inside the dongleMissing()

	m_manager.onAdd(event);

	m_manager.missing().set();
	CPPUNIT_ASSERT(m_manager.done().tryWait(10000));

	CPPUNIT_ASSERT(m_manager.becameAvailable().tryWait(10000));
	// no we must be inside dongleAvailable()
	m_manager.onRemove(event);

	m_manager.available().set();
	CPPUNIT_ASSERT(m_manager.done().tryWait(10000));

	CPPUNIT_ASSERT(m_manager.becameMissing().tryWait(10000));
	// we are again inside the dongleMissing()
	m_manager.missing().set();
	CPPUNIT_ASSERT(m_manager.done().tryWait(10000));

	CPPUNIT_ASSERT(m_manager.becameMissing().tryWait(10000));
	// we are inside the dongleMissing()
	m_manager.stop();

	m_manager.missing().set();
	CPPUNIT_ASSERT(m_manager.done().tryWait(10000));

	CPPUNIT_ASSERT(m_thread.tryJoin(10000));
}


}
