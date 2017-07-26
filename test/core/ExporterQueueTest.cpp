#include <cppunit/extensions/HelperMacros.h>

#include <Poco/AtomicCounter.h>
#include <Poco/SharedPtr.h>
#include <Poco/Exception.h>
#include <Poco/Event.h>
#include <Poco/Timespan.h>

#include "cppunit/BetterAssert.h"

#include "core/ExporterQueue.h"
#include "model/DeviceID.h"

using namespace std;
using namespace BeeeOn;
using namespace Poco;

namespace BeeeOn {

class QueueTestingExporter : public Exporter {
public:
	QueueTestingExporter(function<bool ()> function = &QueueTestingExporter::shipOK):
		m_shipped(0)
	{
		m_ship = function;
	}

	bool ship(const SensorData& data) override
	{
		if (m_ship()) {
			++m_shipped;
			m_lastShipped = data;
			return true;
		}
		else {
			return false;
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
		m_ship = &QueueTestingExporter::shipOK;
	}

	void setFull()
	{
		m_ship = &QueueTestingExporter::shipFull;
	}

	void setBroken()
	{
		m_ship = &QueueTestingExporter::shipBroken;
	}

	AtomicCounter m_shipped;
	SensorData m_lastShipped;

private:
	function<bool ()> m_ship;
};

class ExporterQueueTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ExporterQueueTest);
	CPPUNIT_TEST(testExportOk);
	CPPUNIT_TEST(testQueueOverloaded);
	CPPUNIT_TEST(testExporterBroken);
	CPPUNIT_TEST(testExporterFull);
	CPPUNIT_TEST_SUITE_END();

public:
	void testExportOk();
	void testQueueOverloaded();
	void testExporterBroken();
	void testExporterFull();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ExporterQueueTest);

/**
 * The test vertifies that when exporter set to ExporterQueue successfully
 * exports data, data sent to queue are delivered to exporter once ExporterQueue
 * method exportBatch is called.
 */
void ExporterQueueTest::testExportOk()
{
	SharedPtr<Exporter> exporter = new QueueTestingExporter;

	ExporterQueue queue(exporter, 10, 20, 0);

	DeviceID id01(0x1111222233334444UL);

	for (int i = 0; i < 10; ++i) {
		SensorData data;
		data.setDeviceID(id01);
		queue.enqueue(data);
	}

	// 10 pieces of SensorData are enqueued
	CPPUNIT_ASSERT_EQUAL(10, queue.exportBatch());

	// there is nothing to export
	CPPUNIT_ASSERT_EQUAL(0, queue.exportBatch());

	CPPUNIT_ASSERT_EQUAL(10, exporter.cast<QueueTestingExporter>()->m_shipped);
	CPPUNIT_ASSERT_EQUAL(
		id01,
		exporter.cast<QueueTestingExporter>()->m_lastShipped.deviceID()
	);

	DeviceID id02(0x1111222233335555UL);

	for (int i = 0; i < 20; ++i) {
		SensorData data;
		data.setDeviceID(id02);
		queue.enqueue(data);
	}

	// 20 pieces of SensorData are enqueued, now export them in batches
	CPPUNIT_ASSERT_EQUAL(10, queue.exportBatch());
	CPPUNIT_ASSERT_EQUAL(10, queue.exportBatch());
	CPPUNIT_ASSERT_EQUAL(0, queue.exportBatch());

	// we have exported 10 and then 20 SensorData pieces in total
	CPPUNIT_ASSERT_EQUAL(30, exporter.cast<QueueTestingExporter>()->m_shipped);
	CPPUNIT_ASSERT_EQUAL(
		id02,
		exporter.cast<QueueTestingExporter>()->m_lastShipped.deviceID()
	);
}

/**
 * The test vertifies that when the ExporterQueue is overloaded (its capacity
 * is reached), oldest data are dropped and once ExporterQueue method
 * exportBatch is called enough times, all data are delivered to queue's exporter.
 */
void ExporterQueueTest::testQueueOverloaded()
{
	SharedPtr<Exporter> exporter = new QueueTestingExporter;

	ExporterQueue queue(exporter, 10, 20, 0);

	DeviceID id01(0x1111222233334444UL);

	for (int i = 0; i < 20; ++i) {
		SensorData data;
		data.setDeviceID(id01);
		queue.enqueue(data);
	}

	DeviceID id02(0x1111222233335555UL);

	// overload the queue by 3 more items to drop 3 oldest ones
	for (int i = 0; i < 3; ++i) {
		SensorData data;
		data.setDeviceID(id02);
		queue.enqueue(data);
	}

	CPPUNIT_ASSERT_EQUAL(10, queue.exportBatch());
	CPPUNIT_ASSERT_EQUAL(10, queue.exportBatch());
	CPPUNIT_ASSERT_EQUAL(0, queue.exportBatch());

	// 3 SensorData have been dropped
	CPPUNIT_ASSERT_EQUAL(20, exporter.cast<QueueTestingExporter>()->m_shipped);
	CPPUNIT_ASSERT_EQUAL(
		id02,
		exporter.cast<QueueTestingExporter>()->m_lastShipped.deviceID()
	);
}

/**
 * The test vertifies that when the used exporter fails to export data and fails
 * multiple times while reaching the ExporterQueue::threshold(), the queue changes
 * its status to "not working". When exporter is again successfully
 * exporting data, queue changes status to "working" after the first successfull
 * export.
 * Treshold is set to 0 for this test.
 * "deadTimeout" is 5 seconds because it is assumed that time between calling
 * ExporterQueue::exportBatch() and ExporterQueue::canExport() in this test will
 * be (much) shorter than 5 seconds.
 */
void ExporterQueueTest::testExporterBroken()
{
	SharedPtr<Exporter> exporter = new QueueTestingExporter(&QueueTestingExporter::shipBroken);

	ExporterQueue queue(exporter, 10, 20, 0);

	SensorData data;

	DeviceID id(0x1111222233334444UL);

	data.setDeviceID(id);

	// enqueue data that would fail to be exported
	queue.enqueue(data);

	CPPUNIT_ASSERT_EQUAL(0, queue.exportBatch());
	CPPUNIT_ASSERT_EQUAL(0, exporter.cast<QueueTestingExporter>()->m_shipped);

	// we are dead for much less then 5 seconds
	Timespan deadTimeout(5 * Timespan::SECONDS);
	CPPUNIT_ASSERT(!queue.canExport(deadTimeout));

	exporter.cast<QueueTestingExporter>()->setOK();

	CPPUNIT_ASSERT_EQUAL(1, queue.exportBatch());
	CPPUNIT_ASSERT_EQUAL(1, exporter.cast<QueueTestingExporter>()->m_shipped);

	CPPUNIT_ASSERT(queue.working());

	CPPUNIT_ASSERT_EQUAL(
		id,
		exporter.cast<QueueTestingExporter>()->m_lastShipped.deviceID()
	);
}

/**
 * The test vertifies that when the used exporter fails to export data
 * because it is full, it does not affect queue working status.
 * Treshold is set to 0 for this test.
 */
void ExporterQueueTest::testExporterFull()
{
	SharedPtr<Exporter> exporter = new QueueTestingExporter(&QueueTestingExporter::shipFull);

	ExporterQueue queue(exporter, 10, 20, 0);

	SensorData data;
	DeviceID id(0x1111222233334444UL);
	data.setDeviceID(id);
	queue.enqueue(data);

	CPPUNIT_ASSERT_EQUAL(0, queue.exportBatch());
	CPPUNIT_ASSERT_EQUAL(0, exporter.cast<QueueTestingExporter>()->m_shipped);

	Timespan deadTimeout(5 * Timespan::SECONDS);

	// we are not dead, just non-empty with a temporarily failing exporter
	CPPUNIT_ASSERT(queue.canExport(deadTimeout));

	exporter.cast<QueueTestingExporter>()->setOK();

	CPPUNIT_ASSERT_EQUAL(1, queue.exportBatch());
	CPPUNIT_ASSERT_EQUAL(1, exporter.cast<QueueTestingExporter>()->m_shipped);

	CPPUNIT_ASSERT_EQUAL(
		id,
		exporter.cast<QueueTestingExporter>()->m_lastShipped.deviceID()
	);
}

}
