#include <queue>

#include <cppunit/extensions/HelperMacros.h>

#include "commands/ServerDeviceListResult.h"
#include "core/AnswerQueue.h"
#include "cppunit/BetterAssert.h"
#include "gwmessage/GWNewDeviceRequest.h"
#include "gwmessage/GWDeviceListRequest.h"
#include "gwmessage/GWDeviceListResponse.h"
#include "server/GWSCommandHandler.h"
#include "server/MockGWSConnector.h"
#include "util/NonAsyncExecutor.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class GWSCommandHandlerTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(GWSCommandHandlerTest);
	CPPUNIT_TEST(testHandleNewDevice);
	CPPUNIT_TEST(testHandleServerDeviceList);
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
	void testHandleNewDevice();
	void testHandleServerDeviceList();

private:
	SharedPtr<AnswerQueue> m_queue;
	NonAsyncExecutor::Ptr m_executor;
	MockGWSConnector::Ptr m_connector;
	GWSCommandHandler::Ptr m_handler;
};

CPPUNIT_TEST_SUITE_REGISTRATION(GWSCommandHandlerTest);

template <typename Request>
class AbstractResponder : public GWSListener {
public:
	AbstractResponder(MockGWSConnector &connector):
		m_connector(connector)
	{
	}

	void onSent(GWMessage::Ptr message) override
	{
		typename Request::Ptr request = message.cast<Request>();
		CPPUNIT_ASSERT(!request.isNull());
		m_requests.emplace(request);
	}

	virtual void deliverResponses() = 0;

protected:
	MockGWSConnector &m_connector;
	queue<typename Request::Ptr> m_requests;
};

void GWSCommandHandlerTest::setUp()
{
	m_queue = new AnswerQueue;
	m_executor = new NonAsyncExecutor;
	m_connector = new MockGWSConnector;
	m_handler = new GWSCommandHandler;

	m_connector->setEventsExecutor(m_executor);
	m_connector->addListener(m_handler);
	m_handler->setConnector(m_connector);
}

class NewDeviceResponder : public AbstractResponder<GWNewDeviceRequest> {
public:
	typedef SharedPtr<NewDeviceResponder> Ptr;

	NewDeviceResponder(MockGWSConnector &connector):
		AbstractResponder<GWNewDeviceRequest>(connector)
	{
	}

	void deliverResponses() override
	{
		while (!m_requests.empty()) {
			GWNewDeviceRequest::Ptr request = m_requests.front();
			m_requests.pop();

			m_connector.receive(request->derive([](GWResponse::Ptr r) {
				r->setStatus(GWResponse::Status::SUCCESS);
			}));
		}
	}
};

void GWSCommandHandlerTest::testHandleNewDevice()
{
	NewDeviceResponder::Ptr responder = new NewDeviceResponder(*m_connector);
	m_connector->addListener(responder);

	NewDeviceCommand::Ptr cmd = new NewDeviceCommand(
		DeviceDescription::Builder()
			.id(0xa300000001020304)
			.type("test", "test device")
			.build());
	Answer::Ptr answer = new Answer(*m_queue);
	answer->setHandlersCount(1);

	CPPUNIT_ASSERT(m_handler->accept(cmd));
	m_handler->handle(cmd, answer);

	CPPUNIT_ASSERT(answer->isPending());
	CPPUNIT_ASSERT_EQUAL(1, answer->handlersCount());
	CPPUNIT_ASSERT_EQUAL(1, answer->resultsCount());

	responder->deliverResponses();

	CPPUNIT_ASSERT(!answer->isPending());
	CPPUNIT_ASSERT_EQUAL(1, answer->handlersCount());
	CPPUNIT_ASSERT_EQUAL(1, answer->resultsCount());

	CPPUNIT_ASSERT_EQUAL(Result::Status::SUCCESS, answer->at(0)->status());
}

class ServerDeviceListResponder : public AbstractResponder<GWDeviceListRequest> {
public:
	typedef SharedPtr<ServerDeviceListResponder> Ptr;

	ServerDeviceListResponder(MockGWSConnector &connector):
		AbstractResponder<GWDeviceListRequest>(connector)
	{
	}

	void deliverResponses() override
	{
		while (!m_requests.empty()) {
			GWDeviceListRequest::Ptr request = m_requests.front();
			m_requests.pop();

			m_connector.receive(request->derive<GWDeviceListResponse>(
				[](GWDeviceListResponse::Ptr r) {
					r->setStatus(GWResponse::Status::SUCCESS);
					r->setDevices({
						0xa300000000000001,
						0xa300000000000002,
						0xa300000000000003
					});
				}
			));
		}
	}
};

void GWSCommandHandlerTest::testHandleServerDeviceList()
{
	ServerDeviceListResponder::Ptr responder = new ServerDeviceListResponder(*m_connector);
	m_connector->addListener(responder);

	ServerDeviceListCommand::Ptr cmd = new ServerDeviceListCommand(DevicePrefix::PREFIX_VIRTUAL_DEVICE);
	Answer::Ptr answer = new Answer(*m_queue);
	answer->setHandlersCount(1);

	CPPUNIT_ASSERT(m_handler->accept(cmd));
	m_handler->handle(cmd, answer);

	CPPUNIT_ASSERT(answer->isPending());
	CPPUNIT_ASSERT_EQUAL(1, answer->handlersCount());
	CPPUNIT_ASSERT_EQUAL(1, answer->resultsCount());

	responder->deliverResponses();

	CPPUNIT_ASSERT(!answer->isPending());
	CPPUNIT_ASSERT_EQUAL(1, answer->handlersCount());
	CPPUNIT_ASSERT_EQUAL(1, answer->resultsCount());

	CPPUNIT_ASSERT_EQUAL(Result::Status::SUCCESS, answer->at(0)->status());
	ServerDeviceListResult::Ptr result = answer->at(0).cast<ServerDeviceListResult>();
	CPPUNIT_ASSERT(!result.isNull());
	CPPUNIT_ASSERT_EQUAL(3, result->deviceList().size());
	CPPUNIT_ASSERT(DeviceID(0xa300000000000001) == result->deviceList()[0]);
	CPPUNIT_ASSERT(DeviceID(0xa300000000000002) == result->deviceList()[1]);
	CPPUNIT_ASSERT(DeviceID(0xa300000000000003) == result->deviceList()[2]);
}

}
