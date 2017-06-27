#include <functional>

#include <cppunit/extensions/HelperMacros.h>

#include <Poco/AtomicCounter.h>
#include <Poco/Event.h>
#include <Poco/Exception.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>

#include "cppunit/BetterAssert.h"

#include "core/QueuingDistributor.h"
#include "loop/LoopRunner.h"
#include "model/DeviceID.h"

using namespace std;
using namespace BeeeOn;
using namespace Poco;

namespace BeeeOn {

class TestingExporter : public Exporter {
public:
	TestingExporter(function<bool ()> function = &TestingExporter::shipOK):
		m_shipped(0),
		m_lastShipped(),
		m_shipSet(true),
		m_shipEnabled(false),
		m_shipAttempt(true)
	{
		m_ship = function;
		m_shipAttempt.reset();
		m_shipEnabled.set();
		m_shipSet.reset();
	}

	bool ship(const SensorData& data) override
	{
		if (!m_shipEnabled.tryWait(10000))
			return false;

		try {
			FastMutex::ScopedLock lock(m_functMutex);
			if (m_ship()) {
				++m_shipped;
				m_lastShipped = data;
				m_shipAttempt.set();
				return true;
			}
			else {
				m_shipAttempt.set();
				return false;
			}
		}
		catch (...) {
			m_shipAttempt.set();
			throw;
		}
	}

	static bool shipOK()
	{
		return true;
	}

	static bool shipFull()
	{
		return false;
	}

	static bool shipBroken()
	{
		throw ExistsException("no connection");
	}

	void setOK()
	{
		FastMutex::ScopedLock lock(m_functMutex);
		m_ship = &TestingExporter::shipOK;
		m_shipSet.set();
	}

	void setFull()
	{
		FastMutex::ScopedLock lock(m_functMutex);
		m_ship = &TestingExporter::shipFull;
		m_shipSet.set();
	}

	void setBroken()
	{
		FastMutex::ScopedLock lock(m_functMutex);
		m_ship = &TestingExporter::shipBroken;
		m_shipSet.set();
	}

	bool waitShipAttempt(int seconds = 20)
	{
		return m_shipAttempt.tryWait(seconds * 1000);
	}

	bool waitShipSetAndAttempt(int seconds = 20)
	{
		return m_shipSet.tryWait(seconds * 1000) && m_shipAttempt.tryWait(seconds * 1000);
	}

	void disableShipping()
	{
		m_shipAttempt.reset();
		m_shipEnabled.reset();
	}

	void enableShipping()
	{
		m_shipEnabled.set();
	}

	AtomicCounter m_shipped;
	SensorData m_lastShipped;

private:
	Event m_shipSet;
	Event m_shipEnabled;
	Event m_shipAttempt;
	FastMutex m_functMutex;
	function<bool ()> m_ship;
};


class QueuingDistributorTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(QueuingDistributorTest);
	CPPUNIT_TEST(testExportIsOk);
	CPPUNIT_TEST(testFullExporter);
	CPPUNIT_TEST(testNoConnectivityExporter);
	CPPUNIT_TEST_SUITE_END();

public:
	void testExportIsOk();
	void testFullExporter();
	void testNoConnectivityExporter();

	LoopRunner m_loopRunner;
};

CPPUNIT_TEST_SUITE_REGISTRATION(QueuingDistributorTest);

/**
 * The test vertifies that when exporters registered to QueuingDistributor
 * are successfully exporting data, data sent to distributor, are delivered
 * to all exporters.
 */
void QueuingDistributorTest::testExportIsOk()
{
	SharedPtr<QueuingDistributor> distributor = new QueuingDistributor;
	SharedPtr<Exporter> exporter1 = new TestingExporter;
	SharedPtr<Exporter> exporter2 = new TestingExporter;

	distributor->registerExporter(exporter1);
	distributor->registerExporter(exporter2);

	m_loopRunner.addRunnable(distributor);
	m_loopRunner.start();

	SensorData data;
	DeviceID id(0x1111222233334444UL);
	data.setDeviceID(id);
	distributor->exportData(data);

	CPPUNIT_ASSERT(exporter1.cast<TestingExporter>()->waitShipAttempt());
	CPPUNIT_ASSERT(exporter2.cast<TestingExporter>()->waitShipAttempt());

	CPPUNIT_ASSERT_EQUAL(1, exporter1.cast<TestingExporter>()->m_shipped);
	CPPUNIT_ASSERT_EQUAL(1, exporter2.cast<TestingExporter>()->m_shipped);

	CPPUNIT_ASSERT_EQUAL(
		id,
		exporter1.cast<TestingExporter>()->m_lastShipped.deviceID()
	);
	CPPUNIT_ASSERT_EQUAL(
		id,
		exporter2.cast<TestingExporter>()->m_lastShipped.deviceID()
	);

	m_loopRunner.stop();
}

/**
 * The test vertifies that when exporters registered to QueuingDistributor
 * are full, data sent to distributor are delivered to exporters when they become
 * "not full".
 * deadTimeout is set to 0 to avoid long waiting for attempt to sent data to
 * a broken exporter.
 */
void QueuingDistributorTest::testFullExporter()
{
	SharedPtr<QueuingDistributor> distributor = new QueuingDistributor;
	SharedPtr<Exporter> exporter1 = new TestingExporter(&TestingExporter::shipFull);
	SharedPtr<Exporter> exporter2 = new TestingExporter(&TestingExporter::shipFull);

	distributor->setDeadTimeout(0);
	distributor->registerExporter(exporter1);
	distributor->registerExporter(exporter2);

	m_loopRunner.addRunnable(distributor);
	m_loopRunner.start();

	SensorData data;
	DeviceID id(0x1111222233334444UL);
	data.setDeviceID(id);
	distributor->exportData(data);

	CPPUNIT_ASSERT(exporter1.cast<TestingExporter>()->waitShipAttempt());
	CPPUNIT_ASSERT(exporter2.cast<TestingExporter>()->waitShipAttempt());

	CPPUNIT_ASSERT_EQUAL(0, exporter1.cast<TestingExporter>()->m_shipped);
	CPPUNIT_ASSERT_EQUAL(0, exporter2.cast<TestingExporter>()->m_shipped);

	exporter1.cast<TestingExporter>()->disableShipping();
	exporter2.cast<TestingExporter>()->disableShipping();

	exporter1.cast<TestingExporter>()->setOK();
	exporter2.cast<TestingExporter>()->setOK();

	exporter1.cast<TestingExporter>()->enableShipping();
	exporter2.cast<TestingExporter>()->enableShipping();

	CPPUNIT_ASSERT(exporter1.cast<TestingExporter>()->waitShipSetAndAttempt());
	CPPUNIT_ASSERT(exporter2.cast<TestingExporter>()->waitShipSetAndAttempt());

	CPPUNIT_ASSERT_EQUAL(1, exporter1.cast<TestingExporter>()->m_shipped);
	CPPUNIT_ASSERT_EQUAL(1, exporter2.cast<TestingExporter>()->m_shipped);

	CPPUNIT_ASSERT_EQUAL(
		id,
		exporter1.cast<TestingExporter>()->m_lastShipped.deviceID()
	);
	CPPUNIT_ASSERT_EQUAL(
		id,
		exporter2.cast<TestingExporter>()->m_lastShipped.deviceID()
	);

	m_loopRunner.stop();
}

/**
 * The test vertifies that when exporters registered to QueuingDistributor
 * lost connectivity, data sent to distributor are delivered to exporters when
 * connection is renewed.
 * deadTimeout is set to 0 to avoid long waiting for attempt to sent data to
 * a broken exporter.
 */
void QueuingDistributorTest::testNoConnectivityExporter()
{
	SharedPtr<QueuingDistributor> distributor = new QueuingDistributor;
	SharedPtr<Exporter> exporter1 = new TestingExporter(&TestingExporter::shipBroken);
	SharedPtr<Exporter> exporter2 = new TestingExporter(&TestingExporter::shipBroken);

	distributor->setDeadTimeout(0);
	distributor->registerExporter(exporter1);
	distributor->registerExporter(exporter2);

	m_loopRunner.addRunnable(distributor);
	m_loopRunner.start();

	SensorData data;
	DeviceID id(0x1111222233334444UL);
	data.setDeviceID(id);
	distributor->exportData(data);

	CPPUNIT_ASSERT(exporter1.cast<TestingExporter>()->waitShipAttempt());
	CPPUNIT_ASSERT(exporter2.cast<TestingExporter>()->waitShipAttempt());

	CPPUNIT_ASSERT_EQUAL(0, exporter1.cast<TestingExporter>()->m_shipped);
	CPPUNIT_ASSERT_EQUAL(0, exporter2.cast<TestingExporter>()->m_shipped);

	exporter1.cast<TestingExporter>()->disableShipping();
	exporter2.cast<TestingExporter>()->disableShipping();

	exporter1.cast<TestingExporter>()->setOK();
	exporter2.cast<TestingExporter>()->setOK();

	exporter1.cast<TestingExporter>()->enableShipping();
	exporter2.cast<TestingExporter>()->enableShipping();

	CPPUNIT_ASSERT(exporter1.cast<TestingExporter>()->waitShipSetAndAttempt());
	CPPUNIT_ASSERT(exporter2.cast<TestingExporter>()->waitShipSetAndAttempt());

	CPPUNIT_ASSERT_EQUAL(1, exporter1.cast<TestingExporter>()->m_shipped);
	CPPUNIT_ASSERT_EQUAL(1, exporter2.cast<TestingExporter>()->m_shipped);

	CPPUNIT_ASSERT_EQUAL(
		id,
		exporter1.cast<TestingExporter>()->m_lastShipped.deviceID()
	);
	CPPUNIT_ASSERT_EQUAL(
		id,
		exporter2.cast<TestingExporter>()->m_lastShipped.deviceID()
	);

	m_loopRunner.stop();
}

}
