#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"
#include "iqrf/DPAResponse.h"
#include "iqrf/DPARequest.h"
#include "iqrf/IQRFEvent.h"
#include "iqrf/request/DPAOSPeripheralInfoRequest.h"
#include "iqrf/request/DPACoordBondedNodesRequest.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class IQRFEventTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(IQRFEventTest);
	CPPUNIT_TEST(testEventFromRequestManager);
	CPPUNIT_TEST(testEventFromRequestDevice);
	CPPUNIT_TEST(testEventFromResponseManager);
	CPPUNIT_TEST(testEventFromResponseDevice);
	CPPUNIT_TEST_SUITE_END();

public:
	void testEventFromRequestManager();
	void testEventFromRequestDevice();
	void testEventFromResponseManager();
	void testEventFromResponseDevice();
};

CPPUNIT_TEST_SUITE_REGISTRATION(IQRFEventTest);

void IQRFEventTest::testEventFromRequestManager()
{
	// Raw: 00.00.00.02.ff.ff
	const DPARequest::Ptr request = new DPACoordBondedNodesRequest;
	IQRFEvent event(request);

	CPPUNIT_ASSERT_EQUAL(0x0000, event.networkAddress());
	CPPUNIT_ASSERT_EQUAL(0x00, event.peripheralNumber());
	CPPUNIT_ASSERT_EQUAL(0x02, event.commandCode());
	CPPUNIT_ASSERT_EQUAL(0xffff, event.HWProfile());
}

void IQRFEventTest::testEventFromRequestDevice()
{
	// Raw: 12.00.02.00.ff.ff
	const DPARequest::Ptr request = new DPAOSPeripheralInfoRequest(0x12);
	IQRFEvent event(request);

	CPPUNIT_ASSERT_EQUAL(0x0012, event.networkAddress());
	CPPUNIT_ASSERT_EQUAL(0x02, event.peripheralNumber());
	CPPUNIT_ASSERT_EQUAL(0x00, event.commandCode());
	CPPUNIT_ASSERT_EQUAL(0xffff, event.HWProfile());
}

void IQRFEventTest::testEventFromResponseManager()
{
	const DPAResponse::Ptr response = DPAResponse::fromRaw(
		"00.01.00.85.ff.ff.00.00.07");
	IQRFEvent event(response);

	CPPUNIT_ASSERT_EQUAL(0x0100, event.networkAddress());
	CPPUNIT_ASSERT_EQUAL(0x00, event.peripheralNumber());
	CPPUNIT_ASSERT_EQUAL(0x85, event.commandCode());
	CPPUNIT_ASSERT_EQUAL(0xffff, event.HWProfile());

	CPPUNIT_ASSERT_EQUAL(1, event.size());
	CPPUNIT_ASSERT_EQUAL(event.payload().size(), event.size());
	CPPUNIT_ASSERT_EQUAL(0x07, event.payload()[0]);
}

void IQRFEventTest::testEventFromResponseDevice()
{
	const DPAResponse::Ptr response = DPAResponse::fromRaw(
		"00.00.02.80.ff.ff.00.00.E4.57.00.81.42.B4.B8.08.5C.1F.00.85");
	IQRFEvent event(response);

	CPPUNIT_ASSERT_EQUAL(0x0000, event.networkAddress());
	CPPUNIT_ASSERT_EQUAL(0x02, event.peripheralNumber());
	CPPUNIT_ASSERT_EQUAL(0x80, event.commandCode());
	CPPUNIT_ASSERT_EQUAL(0xffff, event.HWProfile());

}

}
