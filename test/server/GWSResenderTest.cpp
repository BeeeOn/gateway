#include <list>

#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"
#include "gwmessage/GWNewDeviceRequest.h"
#include "gwmessage/GWResponseWithAck.h"
#include "gwmessage/GWSensorDataConfirm.h"
#include "gwmessage/GWSensorDataExport.h"
#include "server/GWSResender.h"
#include "server/MockGWSConnector.h"
#include "util/NonAsyncExecutor.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class TestableGWSResender : public GWSResender {
public:
	typedef SharedPtr<TestableGWSResender> Ptr;

	using GWSResender::resendOrGet;
	using GWSResender::waiting;
};

class GWSResenderTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(GWSResenderTest);
	CPPUNIT_TEST(testNoResendWouldOccur);
	CPPUNIT_TEST(testConfirmEarly);
	CPPUNIT_TEST(testResendRequest);
	CPPUNIT_TEST(testResendResponseWithAck);
	CPPUNIT_TEST(testResendSensorData);
	CPPUNIT_TEST(testResendAcceptSuccess);
	CPPUNIT_TEST(testResendAcceptFailure);
	CPPUNIT_TEST(testResendSuccessFailureBug);
	CPPUNIT_TEST(testResendFailureSuccessBug);
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
	void tearDown();

	void testNoResendWouldOccur();
	void testConfirmEarly();
	void testResendRequest();
	void testResendResponseWithAck();
	void testResendSensorData();
	void testResendAcceptSuccess();
	void testResendAcceptFailure();
	void testResendSuccessFailureBug();
	void testResendFailureSuccessBug();

private:
	NonAsyncExecutor::Ptr m_executor;
	MockGWSConnector::Ptr m_connector;
	TestableGWSResender::Ptr m_resender;
};

CPPUNIT_TEST_SUITE_REGISTRATION(GWSResenderTest);

class SentWatcher : public GWSListener {
public:
	typedef SharedPtr<SentWatcher> Ptr;

	void onSent(const GWMessage::Ptr message) override
	{
		m_sent.emplace_back(message->id());
	}

	list<GlobalID> &sent()
	{
		return m_sent;
	}

private:
	list<GlobalID> m_sent;
};

void GWSResenderTest::setUp()
{
	m_executor = new NonAsyncExecutor;
	m_connector = new MockGWSConnector;

	m_connector->setEventsExecutor(m_executor);

	m_resender = new TestableGWSResender;
	m_resender->setConnector(m_connector);
	m_resender->setResendTimeout(30 * Timespan::SECONDS);

	m_connector->addListener(m_resender);
}

void GWSResenderTest::tearDown()
{
	m_connector->clearListeners();
}

/**
 * @brief Test that a confirmed sensor data export is not resent.
 */
void GWSResenderTest::testNoResendWouldOccur()
{
	GWSensorDataExport::Ptr request = new GWSensorDataExport;
	request->setID(GlobalID::parse("d1e302a5-c672-40a5-a1ab-18a8e5ec259e"));

	GWSensorDataConfirm::Ptr confirm = request->confirm();

	m_resender->onTrySend(request);

	auto it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it == end(m_resender->waiting()));

	m_resender->onSent(request);

	it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it != end(m_resender->waiting()));
	CPPUNIT_ASSERT(it->second->id() == request->id());

	m_resender->onOther(confirm);

	it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it == end(m_resender->waiting()));
}

/**
 * @brief Test situation when the sensor data export is confirmed
 * faster then the event is delivered to GWSResender instance.
 * It must not be scheduled for resent.
 */
void GWSResenderTest::testConfirmEarly()
{
	GWSensorDataExport::Ptr request = new GWSensorDataExport;
	request->setID(GlobalID::parse("d1e302a5-c672-40a5-a1ab-18a8e5ec259e"));

	GWSensorDataConfirm::Ptr confirm = request->confirm();

	m_resender->onTrySend(request);

	auto it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it == end(m_resender->waiting()));

	m_resender->onOther(confirm);

	it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it == end(m_resender->waiting()));

	m_resender->onSent(request);

	it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it == end(m_resender->waiting()));
}

/**
 * @brief Test that request with no response within the
 * resendTimeout is resent and it is being resent until
 * the response comes.
 */
void GWSResenderTest::testResendRequest()
{
	SentWatcher::Ptr watcher = new SentWatcher;
	m_connector->addListener(watcher);

	m_resender->setResendTimeout(30 * Timespan::SECONDS);

	GWNewDeviceRequest::Ptr request = new GWNewDeviceRequest;
	request->setID(GlobalID::parse("2becaa23-8bdf-4e03-8d85-49ab4bac2a0e"));
	request->setDeviceID(0xa300000000000001);
	request->setProductName("some device");
	request->setVendor("Magic Company");

	m_resender->onTrySend(request);

	auto it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it == end(m_resender->waiting()));

	m_resender->onSent(request);

	CPPUNIT_ASSERT(watcher->sent().empty());

	it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it->second->id() == request->id());
	CPPUNIT_ASSERT(watcher->sent().empty());

	for (size_t i = 0; i < 3; ++i) {
		const Clock now;
		it = m_resender->resendOrGet(now + 30 * Timespan::SECONDS);

		CPPUNIT_ASSERT(it != end(m_resender->waiting()));
		CPPUNIT_ASSERT(it->second->id() == request->id());
		CPPUNIT_ASSERT_EQUAL(1 + i, watcher->sent().size());
	}

	GWResponse::Ptr response = request->derive();
	response->setStatus(GWResponse::Status::SUCCESS);
	m_resender->onResponse(response);
	it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it == end(m_resender->waiting()));
}

/**
 * @brief Test that response (with ack) with no ack within the
 * resendTimeout is resent and it is being resent until the
 * appropriate ack comes.
 */
void GWSResenderTest::testResendResponseWithAck()
{
	SentWatcher::Ptr watcher = new SentWatcher;
	m_connector->addListener(watcher);

	m_resender->setResendTimeout(30 * Timespan::SECONDS);

	GWResponseWithAck::Ptr response = new GWResponseWithAck;
	response->setID(GlobalID::parse("e48d03e7-56b8-45fc-bb83-2f49e3f4f338"));
	response->setStatus(GWResponse::Status::SUCCESS);

	m_resender->onTrySend(response);

	auto it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it == end(m_resender->waiting()));

	m_resender->onSent(response);

	CPPUNIT_ASSERT(watcher->sent().empty());

	it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it->second->id() == response->id());
	CPPUNIT_ASSERT(watcher->sent().empty());

	for (size_t i = 0; i < 3; ++i) {
		const Clock now;
		it = m_resender->resendOrGet(now + 30 * Timespan::SECONDS);

		CPPUNIT_ASSERT(it != end(m_resender->waiting()));
		CPPUNIT_ASSERT(it->second->id() == response->id());
		CPPUNIT_ASSERT_EQUAL(1 + i, watcher->sent().size());
	}

	m_resender->onAck(response->ack());
	it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it == end(m_resender->waiting()));
}

/**
 * @brief Test that sensor data export with no confirmation within
 * the resentTimeout is resent and it is being resent until the
 * appropriate confirmation comes.
 */
void GWSResenderTest::testResendSensorData()
{
	SentWatcher::Ptr watcher = new SentWatcher;
	m_connector->addListener(watcher);

	m_resender->setResendTimeout(30 * Timespan::SECONDS);

	GWSensorDataExport::Ptr request = new GWSensorDataExport;
	request->setID(GlobalID::parse("5ca93c7c-6b08-40ac-a9e2-2f985d3b0580"));

	m_resender->onTrySend(request);

	auto it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it == end(m_resender->waiting()));

	m_resender->onSent(request);

	CPPUNIT_ASSERT(watcher->sent().empty());

	it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it->second->id() == request->id());
	CPPUNIT_ASSERT(watcher->sent().empty());

	for (size_t i = 0; i < 3; ++i) {
		const Clock now;
		it = m_resender->resendOrGet(now + 30 * Timespan::SECONDS);

		CPPUNIT_ASSERT(it != end(m_resender->waiting()));
		CPPUNIT_ASSERT(it->second->id() == request->id());
		CPPUNIT_ASSERT_EQUAL(1 + i, watcher->sent().size());
	}

	GWSensorDataConfirm::Ptr confirm = request->confirm();
	m_resender->onOther(confirm);
	it = m_resender->resendOrGet({});
	CPPUNIT_ASSERT(it == end(m_resender->waiting()));

}

/**
 * @brief When attempting to resend a response with status ACCEPT and
 * then another one of the same ID with status SUCCESS, only the second
 * one would be resent. Thus, ack on the ACCEPT response is ignored.
 */
void GWSResenderTest::testResendAcceptSuccess()
{
	GWResponseWithAck::Ptr accept = new GWResponseWithAck;
	accept->setID(GlobalID::parse("eb217793-6827-4ee0-9d89-3b1bde66bcde"));
	accept->setStatus(GWResponse::Status::ACCEPTED);

	GWResponseWithAck::Ptr success = new GWResponseWithAck;
	success->setID(GlobalID::parse("eb217793-6827-4ee0-9d89-3b1bde66bcde"));
	success->setStatus(GWResponse::Status::SUCCESS);

	m_resender->onTrySend(accept);
	m_resender->onSent(accept);
	CPPUNIT_ASSERT_EQUAL(1, m_resender->waiting().size());

	const GWResponse::Ptr tmp0 = m_resender->waiting()
		.begin()->second
		.cast<GWResponse>();
	CPPUNIT_ASSERT_EQUAL(GWResponse::Status::ACCEPTED, tmp0->status());

	m_resender->onTrySend(success);
	m_resender->onSent(success);
	CPPUNIT_ASSERT_EQUAL(1, m_resender->waiting().size());

	const GWResponse::Ptr tmp1 = m_resender->waiting()
		.begin()->second
		.cast<GWResponse>();
	CPPUNIT_ASSERT_EQUAL(GWResponse::Status::SUCCESS, tmp1->status());

	// ack of ACCEPTED would be ignored
	m_resender->onAck(accept->ack());
	CPPUNIT_ASSERT_EQUAL(1, m_resender->waiting().size());

	const GWResponse::Ptr tmp2 = m_resender->waiting()
		.begin()->second
		.cast<GWResponse>();
	CPPUNIT_ASSERT_EQUAL(GWResponse::Status::SUCCESS, tmp2->status());

	// ack of SUCCESS works
	m_resender->onAck(success->ack());
	CPPUNIT_ASSERT_EQUAL(0, m_resender->waiting().size());
}

/**
 * @brief When attempting to resend a response with status ACCEPT and
 * then another one of the same ID with status FAILED, only the second
 * one would be resent. Thus, ack on the ACCEPT response is ignored.
 */
void GWSResenderTest::testResendAcceptFailure()
{
	GWResponseWithAck::Ptr accept = new GWResponseWithAck;
	accept->setID(GlobalID::parse("d85cb15e-095b-434e-915d-85167c82a070"));
	accept->setStatus(GWResponse::Status::ACCEPTED);

	GWResponseWithAck::Ptr failed = new GWResponseWithAck;
	failed->setID(GlobalID::parse("d85cb15e-095b-434e-915d-85167c82a070"));
	failed->setStatus(GWResponse::Status::FAILED);

	m_resender->onTrySend(accept);
	m_resender->onSent(accept);
	CPPUNIT_ASSERT_EQUAL(1, m_resender->waiting().size());

	const GWResponse::Ptr tmp0 = m_resender->waiting()
		.begin()->second
		.cast<GWResponse>();
	CPPUNIT_ASSERT_EQUAL(GWResponse::Status::ACCEPTED, tmp0->status());

	m_resender->onTrySend(failed);
	m_resender->onSent(failed);
	CPPUNIT_ASSERT_EQUAL(1, m_resender->waiting().size());

	const GWResponse::Ptr tmp1 = m_resender->waiting()
		.begin()->second
		.cast<GWResponse>();
	CPPUNIT_ASSERT_EQUAL(GWResponse::Status::FAILED, tmp1->status());

	// ack of ACCEPTED would be ignored
	m_resender->onAck(accept->ack());
	CPPUNIT_ASSERT_EQUAL(1, m_resender->waiting().size());

	const GWResponse::Ptr tmp2 = m_resender->waiting()
		.begin()->second
		.cast<GWResponse>();
	CPPUNIT_ASSERT_EQUAL(GWResponse::Status::FAILED, tmp2->status());

	// ack of FAILED works
	m_resender->onAck(failed->ack());
	CPPUNIT_ASSERT_EQUAL(0, m_resender->waiting().size());
}

/**
 * @brief In case when for a request is generated a response
 * with status success and as a result of bug or some unexpected
 * behaviour, there is also a response with status failure,
 * the resender must deal with this situation. Only the first
 * response would be used for resend.
 */
void GWSResenderTest::testResendSuccessFailureBug()
{
	GWResponseWithAck::Ptr success = new GWResponseWithAck;
	success->setID(GlobalID::parse("6808d391-5727-4d15-b6b1-05661b4d127b"));
	success->setStatus(GWResponse::Status::SUCCESS);

	GWResponseWithAck::Ptr failed = new GWResponseWithAck;
	failed->setID(GlobalID::parse("6808d391-5727-4d15-b6b1-05661b4d127b"));
	failed->setStatus(GWResponse::Status::FAILED);

	m_resender->onTrySend(success);
	m_resender->onSent(success);
	CPPUNIT_ASSERT_EQUAL(1, m_resender->waiting().size());

	const GWResponse::Ptr tmp0 = m_resender->waiting()
		.begin()->second
		.cast<GWResponse>();
	CPPUNIT_ASSERT_EQUAL(GWResponse::Status::SUCCESS, tmp0->status());

	m_resender->onTrySend(failed);
	m_resender->onSent(failed);
	CPPUNIT_ASSERT_EQUAL(1, m_resender->waiting().size());

	const GWResponse::Ptr tmp1 = m_resender->waiting()
		.begin()->second
		.cast<GWResponse>();
	// success is there and would stay there
	CPPUNIT_ASSERT_EQUAL(GWResponse::Status::SUCCESS, tmp1->status());
}

/**
 * @brief Same as GWSResenderTest::testResendSuccessFailureBug() but reveresed.
 */
void GWSResenderTest::testResendFailureSuccessBug()
{
	GWResponseWithAck::Ptr failed = new GWResponseWithAck;
	failed->setID(GlobalID::parse("6808d391-5727-4d15-b6b1-05661b4d127b"));
	failed->setStatus(GWResponse::Status::FAILED);

	GWResponseWithAck::Ptr success = new GWResponseWithAck;
	success->setID(GlobalID::parse("6808d391-5727-4d15-b6b1-05661b4d127b"));
	success->setStatus(GWResponse::Status::SUCCESS);

	m_resender->onTrySend(failed);
	m_resender->onSent(failed);
	CPPUNIT_ASSERT_EQUAL(1, m_resender->waiting().size());

	const GWResponse::Ptr tmp0 = m_resender->waiting()
		.begin()->second
		.cast<GWResponse>();
	CPPUNIT_ASSERT_EQUAL(GWResponse::Status::FAILED, tmp0->status());

	m_resender->onTrySend(success);
	m_resender->onSent(success);
	CPPUNIT_ASSERT_EQUAL(1, m_resender->waiting().size());

	const GWResponse::Ptr tmp1 = m_resender->waiting()
		.begin()->second
		.cast<GWResponse>();
	// failed is there and would stay there
	CPPUNIT_ASSERT_EQUAL(GWResponse::Status::FAILED, tmp1->status());
}

}
