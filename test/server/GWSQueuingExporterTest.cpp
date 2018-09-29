#include <queue>

#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Event.h>
#include <Poco/Thread.h>

#include "gwmessage/GWSensorDataConfirm.h"
#include "gwmessage/GWSensorDataExport.h"
#include "cppunit/BetterAssert.h"
#include "exporters/InMemoryQueuingStrategy.h"
#include "server/GWSQueuingExporter.h"
#include "server/MockGWSConnector.h"
#include "util/NonAsyncExecutor.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class GWSQueuingExporterTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(GWSQueuingExporterTest);
	CPPUNIT_TEST(testShipAndWaitOneByOne);
	CPPUNIT_TEST(testShipAndWaitBatched);
	CPPUNIT_TEST(testShipNoConfirm);
	CPPUNIT_TEST(testSendFails);
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();

	void testShipAndWaitOneByOne();
	void testShipAndWaitBatched();
	void testShipNoConfirm();
	void testSendFails();

protected:
	void clearConnector();

private:
	NonAsyncExecutor::Ptr m_executor;
	MockGWSConnector::Ptr m_connector;
	InMemoryQueuingStrategy::Ptr m_queuingStrategy;
	GWSQueuingExporter::Ptr m_exporter;
};

CPPUNIT_TEST_SUITE_REGISTRATION(GWSQueuingExporterTest);

class SensorDataConfirmer : public GWSListener {
public:
	typedef Poco::SharedPtr<SensorDataConfirmer> Ptr;

	SensorDataConfirmer(MockGWSConnector &connector):
		m_connector(connector)
	{
	}

	void onSent(const GWMessage::Ptr message) override
	{
		GWSensorDataExport::Ptr request = message.cast<GWSensorDataExport>();
		CPPUNIT_ASSERT(!request.isNull());

		FastMutex::ScopedLock guard(m_lock);
		m_exports.emplace_back(request);

		m_event.set();
	}

	Poco::Event &exportEvent()
	{
		return m_event;
	}

	const list<GWSensorDataExport::Ptr> &exports() const
	{
		return m_exports;
	}

	vector<SensorData> confirmExport()
	{
		FastMutex::ScopedLock guard(m_lock);

		CPPUNIT_ASSERT(!m_exports.empty());
		GWSensorDataExport::Ptr request = m_exports.front();
		m_connector.receive(request->confirm());

		m_exports.pop_front();

		return request->data();
	}

private:
	MockGWSConnector &m_connector;
	list<GWSensorDataExport::Ptr> m_exports;
	Event m_event;
	FastMutex m_lock;
};

void GWSQueuingExporterTest::setUp()
{
	m_executor = new NonAsyncExecutor;
	m_connector = new MockGWSConnector;
	m_queuingStrategy = new InMemoryQueuingStrategy;
	m_exporter = new GWSQueuingExporter;

	m_exporter->setConnector(m_connector);
	m_exporter->setStrategy(m_queuingStrategy);

	m_connector->setEventsExecutor(m_executor);
	m_connector->addListener(m_exporter);
}

void GWSQueuingExporterTest::clearConnector()
{
	m_connector->clearListeners();
	m_exporter = nullptr;
	m_connector = nullptr;
}

static const vector<SensorData> DATA = {
	{
		0xa300000000000001,
		{},
		{{0, 15}, {1, 10}}
	},
	{
		0xa300000000000002,
		{},
		{{0, 1}, {1, 20}, {3, 0}}
	},
	{
		0xa300000000000003,
		{},
		{{0, 1}}
	},
	{
		0xa300000000000001,
		{},
		{{0, 16}, {1, 9}}
	},
	{
		0xa300000000000004,
		{},
		{{0, 5}, {1, 1}}
	},
	{
		0xa300000000000002,
		{},
		{{0, 5}, {1, 21}}
	}
};

/**
 * @brief Check stop-and-wait behaviour by sending sensor data one-by-one
 * and confirm after each one.
 */
void GWSQueuingExporterTest::testShipAndWaitOneByOne()
{
	SensorDataConfirmer::Ptr confirmer = new SensorDataConfirmer(*m_connector);

	m_connector->addListener(confirmer);
	m_exporter->setActiveCount(1);
	m_exporter->setAcquireTimeout(10 * Timespan::MILLISECONDS);

	Thread thread;
	thread.start(*m_exporter);

	for (const auto &one : DATA) {
		CPPUNIT_ASSERT(m_exporter->ship(one));
		CPPUNIT_ASSERT_NO_THROW(confirmer->exportEvent().wait(1000));
		const auto result = confirmer->confirmExport();

		CPPUNIT_ASSERT_EQUAL(1, result.size());
		CPPUNIT_ASSERT(result[0] == one);
	}

	m_exporter->stop();
	thread.join();

	confirmer = nullptr; // ensure save occurs
	clearConnector();
	CPPUNIT_ASSERT(m_queuingStrategy->empty());
}

/**
 * @brief Ship 5 sensor data entries and expect them to be
 * exported first as a batch of 4 entries and then the last one.
 */
void GWSQueuingExporterTest::testShipAndWaitBatched()
{
	SensorDataConfirmer::Ptr confirmer = new SensorDataConfirmer(*m_connector);

	m_connector->addListener(confirmer);
	m_exporter->setActiveCount(4);
	m_exporter->setAcquireTimeout(10 * Timespan::MILLISECONDS);

	for (const auto &one : DATA)
		CPPUNIT_ASSERT(m_exporter->ship(one));

	// start exporter after ship to avoid acquire to happen too early
	Thread thread;
	thread.start(*m_exporter);

	CPPUNIT_ASSERT_NO_THROW(confirmer->exportEvent().wait(1000));
	const auto result0 = confirmer->confirmExport();
	CPPUNIT_ASSERT_EQUAL(4, result0.size());

	size_t i = 0;
	for (const auto &one : result0)
		CPPUNIT_ASSERT(one == DATA[i++]);

	CPPUNIT_ASSERT_NO_THROW(confirmer->exportEvent().wait(1000));
	const auto result1 = confirmer->confirmExport();
	CPPUNIT_ASSERT_EQUAL(2, result1.size());

	CPPUNIT_ASSERT(result1[0] == DATA[4]);

	m_exporter->stop();
	thread.join();

	confirmer = nullptr; // ensure save occurs
	clearConnector();
	CPPUNIT_ASSERT(m_queuingStrategy->empty());
}

/**
 * @brief Export 6 sensor data entries and expect only 1 to be exported.
 * Without any confirmation, the exporter would store 5 or all 6 entries
 * via the memory strategy.
 */
void GWSQueuingExporterTest::testShipNoConfirm()
{
	SensorDataConfirmer::Ptr confirmer = new SensorDataConfirmer(*m_connector);

	m_connector->addListener(confirmer);
	m_exporter->setSaveThreshold(3);
	m_exporter->setActiveCount(1);
	m_exporter->setAcquireTimeout(10 * Timespan::MILLISECONDS);

	CPPUNIT_ASSERT(confirmer->exports().empty());

	for (const auto &one : vector<SensorData>(begin(DATA), begin(DATA) + 3))
		CPPUNIT_ASSERT(m_exporter->ship(one));

	CPPUNIT_ASSERT_EQUAL(3, m_queuingStrategy->size());

	// start exporter after ship to avoid acquire() to
	// be called before the first 3 entries are shipped
	Thread thread;
	thread.start(*m_exporter);

	CPPUNIT_ASSERT_NO_THROW(confirmer->exportEvent().wait(1000));
	CPPUNIT_ASSERT_EQUAL(1, confirmer->exports().size());
	CPPUNIT_ASSERT_EQUAL(1, confirmer->exports().front()->data().size());
	CPPUNIT_ASSERT(DATA[0] == confirmer->exports().front()->data()[0]);

	for (const auto &one : vector<SensorData>(begin(DATA) + 3, end(DATA)))
		CPPUNIT_ASSERT(m_exporter->ship(one));

	m_exporter->stop();
	thread.join();

	confirmer = nullptr; // ensure save occurs
	clearConnector();
	CPPUNIT_ASSERT_EQUAL(6, m_queuingStrategy->size());
}

void GWSQueuingExporterTest::testSendFails()
{
	SensorDataConfirmer::Ptr confirmer = new SensorDataConfirmer(*m_connector);

	m_connector->setSendException(new IOException("connection failed"));
	m_connector->addListener(confirmer);
	m_exporter->setActiveCount(1);

	Thread thread;
	thread.start(*m_exporter);

	CPPUNIT_ASSERT(m_exporter->ship({
		0xa300000000000002,
		{},
		{{0, 5}, {1, 21}}
	}));

	CPPUNIT_ASSERT_THROW(
		confirmer->exportEvent().wait(100),
		TimeoutException);

	m_exporter->stop();
	thread.join();

	confirmer = nullptr; // ensure save occurs
	clearConnector();
	CPPUNIT_ASSERT_EQUAL(1, m_queuingStrategy->size());
}

}
