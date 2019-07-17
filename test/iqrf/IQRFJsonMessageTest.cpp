#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"
#include "iqrf/IQRFJsonRequest.h"
#include "iqrf/IQRFJsonResponse.h"
#include "util/JsonUtil.h"

using namespace std;
using namespace Poco;

namespace BeeeOn {

class IQRFJsonMessageTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(IQRFJsonMessageTest);
	CPPUNIT_TEST(testCreateRequest);
	CPPUNIT_TEST(testParseResponse);
	CPPUNIT_TEST_SUITE_END();

public:
	void testCreateRequest();
	void testParseResponse();

private:
	string jsonReformat(const string &json);
};

CPPUNIT_TEST_SUITE_REGISTRATION(IQRFJsonMessageTest);

string IQRFJsonMessageTest::jsonReformat(const string &json)
{
	return Dynamic::Var::toString(JsonUtil::parse(json));
}

void IQRFJsonMessageTest::testCreateRequest()
{
	IQRFJsonRequest::Ptr msg = new IQRFJsonRequest;

	msg->setMessageID("111");
	msg->setRequest("00.11.12.13.14");
	msg->setTimeout(10 * Timespan::SECONDS);

	CPPUNIT_ASSERT_EQUAL(
  R"({
  "mType" : "iqrfRaw",
  "data" : {
    "msgId" : "111",
    "timeout" : 10000,
    "req" : {
      "rData" : "00.11.12.13.14"
    },
    "returnVerbose" : true
  }
})",
		msg->toString()
	);
}

void IQRFJsonMessageTest::testParseResponse()
{
	IQRFJsonMessage::Ptr msg = IQRFJsonMessage::parse(
		R"({
			"mType": "iqrfRaw",
			"data": {
				"msgId": "111",
				"timeout": 10000,
				"rsp": {
					"rData": "00.11.12.13.14.15"
				},
				"raw": [{
					"request": "00.11.12.13.14",
					"requestTs": "",
					"confirmation": "",
					"confirmationTs": "",
					"response": "00.11.12.13.14.15",
					"responseTs": ""
				}],
				"insId": "iqrfgd2-1",
				"statusStr": "ERROR_TIMEOUT",
				"status": 11
			}
		})");

	auto request = msg.cast<IQRFJsonResponse>();

	CPPUNIT_ASSERT_EQUAL("111", request->messageID());
	CPPUNIT_ASSERT_EQUAL("00.11.12.13.14", request->request());
	CPPUNIT_ASSERT_EQUAL("00.11.12.13.14.15", request->response());
	CPPUNIT_ASSERT_EQUAL(10, request->timeout().seconds());
	CPPUNIT_ASSERT_EQUAL(
		IQRFJsonResponse::DpaError::ERROR_TIMEOUT,
		request->errorCode());
}

}
