#include <vector>

#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"
#include "gwmessage/GWSensorDataExport.h"
#include "model/SensorData.h"
#include "server/GWSOptimisticExporter.h"
#include "server/MockGWSConnector.h"
#include "util/NonAsyncExecutor.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class GWSOptimisticExporterTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(GWSOptimisticExporterTest);
	CPPUNIT_TEST(testShipSuccessful);
	CPPUNIT_TEST(testShipNotConnected);
	CPPUNIT_TEST(testShipWhenReconnected);
	CPPUNIT_TEST(testShipFails);
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
	void tearDown();

	void testShipSuccessful();
	void testShipNotConnected();
	void testShipWhenReconnected();
	void testShipFails();

private:
	NonAsyncExecutor::Ptr m_executor;
	MockGWSConnector::Ptr m_connector;
	GWSOptimisticExporter::Ptr m_exporter;
};

CPPUNIT_TEST_SUITE_REGISTRATION(GWSOptimisticExporterTest);

class SensorDataReceiver : public GWSListener {
public:
	typedef Poco::SharedPtr<SensorDataReceiver> Ptr;

	void onSent(const GWMessage::Ptr message) override
	{
		GWSensorDataExport::Ptr request = message.cast<GWSensorDataExport>();
		CPPUNIT_ASSERT(!request.isNull());

		m_sent.emplace_back(request);
	}

	const vector<SensorData> exported() const
	{
		vector<SensorData> data;

		for (const auto request : m_sent) {
			for (const auto &one : request->data())
				data.emplace_back(one);
		}

		return data;
	}

	void clear()
	{
		m_sent.clear();
	}

private:
	vector<GWSensorDataExport::Ptr> m_sent;
};

void GWSOptimisticExporterTest::setUp()
{
	m_executor = new NonAsyncExecutor;
	m_connector = new MockGWSConnector;
	m_exporter = new GWSOptimisticExporter;

	m_exporter->setConnector(m_connector);

	m_connector->setEventsExecutor(m_executor);
	m_connector->addListener(m_exporter);
}

void GWSOptimisticExporterTest::tearDown()
{
	m_connector->clearListeners();
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
};

/**
 * @brief Ship simply succeeds.
 */
void GWSOptimisticExporterTest::testShipSuccessful()
{
	SensorDataReceiver::Ptr receiver = new SensorDataReceiver;
	m_connector->addListener(receiver);

	m_exporter->setExportNonConfirmed(3);

	m_exporter->onConnected({"127.0.0.1", 9000});
	CPPUNIT_ASSERT(receiver->exported().empty());

	CPPUNIT_ASSERT(m_exporter->ship(DATA[0]));
	CPPUNIT_ASSERT_EQUAL(1, receiver->exported().size());
	CPPUNIT_ASSERT(DATA[0] == receiver->exported()[0]);

	CPPUNIT_ASSERT(m_exporter->ship(DATA[1]));
	CPPUNIT_ASSERT_EQUAL(2, receiver->exported().size());
	CPPUNIT_ASSERT(DATA[0] == receiver->exported()[0]);
	CPPUNIT_ASSERT(DATA[1] == receiver->exported()[1]);

	CPPUNIT_ASSERT(m_exporter->ship(DATA[2]));
	CPPUNIT_ASSERT_EQUAL(3, receiver->exported().size());
	CPPUNIT_ASSERT(DATA[0] == receiver->exported()[0]);
	CPPUNIT_ASSERT(DATA[1] == receiver->exported()[1]);
	CPPUNIT_ASSERT(DATA[2] == receiver->exported()[2]);
}

/**
 * @brief Ship would not succeed because the connector is not connected.
 */
void GWSOptimisticExporterTest::testShipNotConnected()
{
	SensorDataReceiver::Ptr receiver = new SensorDataReceiver;
	m_connector->addListener(receiver);

	CPPUNIT_ASSERT(!m_exporter->ship(DATA[0]));
	CPPUNIT_ASSERT(receiver->exported().empty());
}

/**
 * @brief First ship succeeds but then the connector is disconnected.
 * The second ship must fail. After reconnected, the repeated second
 * ship succeeds.
 */
void GWSOptimisticExporterTest::testShipWhenReconnected()
{
	SensorDataReceiver::Ptr receiver = new SensorDataReceiver;
	m_connector->addListener(receiver);

	m_exporter->setExportNonConfirmed(2);

	m_exporter->onConnected({"127.0.0.1", 9000});
	CPPUNIT_ASSERT(receiver->exported().empty());

	CPPUNIT_ASSERT(m_exporter->ship(DATA[0]));
	CPPUNIT_ASSERT_EQUAL(1, receiver->exported().size());
	CPPUNIT_ASSERT(DATA[0] == receiver->exported()[0]);

	receiver->clear();

	m_exporter->onDisconnected({"127.0.0.1", 9000});
	CPPUNIT_ASSERT(receiver->exported().empty());

	CPPUNIT_ASSERT(!m_exporter->ship(DATA[1]));
	CPPUNIT_ASSERT(receiver->exported().empty());

	m_exporter->onConnected({"127.0.0.1", 9000});
	CPPUNIT_ASSERT(receiver->exported().empty());

	CPPUNIT_ASSERT(m_exporter->ship(DATA[1]));
	CPPUNIT_ASSERT_EQUAL(1, receiver->exported().size());
	CPPUNIT_ASSERT(DATA[1] == receiver->exported()[0]);
}

/**
 * @brief First ship succeeds but then the connector starts failing.
 * The second ship must fail. After the connector works well again, the
 * repeated second ship succeeds.
 */
void GWSOptimisticExporterTest::testShipFails()
{
	SensorDataReceiver::Ptr receiver = new SensorDataReceiver;
	m_connector->addListener(receiver);

	m_exporter->setExportNonConfirmed(2);

	m_exporter->onConnected({"127.0.0.1", 9000});
	CPPUNIT_ASSERT(receiver->exported().empty());

	CPPUNIT_ASSERT(m_exporter->ship(DATA[0]));
	CPPUNIT_ASSERT_EQUAL(1, receiver->exported().size());
	CPPUNIT_ASSERT(DATA[0] == receiver->exported()[0]);

	m_connector->setSendException(new IOException("remote is unreachable"));
	receiver->clear();

	CPPUNIT_ASSERT(!m_exporter->ship(DATA[1]));
	CPPUNIT_ASSERT(receiver->exported().empty());

	m_connector->setSendException(nullptr);

	CPPUNIT_ASSERT(m_exporter->ship(DATA[1]));
	CPPUNIT_ASSERT_EQUAL(1, receiver->exported().size());
	CPPUNIT_ASSERT(DATA[1] == receiver->exported()[0]);
}

}
