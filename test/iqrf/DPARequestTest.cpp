#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"
#include "iqrf/DPARequest.h"
#include "iqrf/request/DPACoordBondedNodesRequest.h"
#include "iqrf/request/DPACoordBondNodeRequest.h"
#include "iqrf/request/DPACoordClearAllBondsRequest.h"
#include "iqrf/request/DPACoordDiscoveryRequest.h"
#include "iqrf/request/DPACoordRemoveNodeRequest.h"
#include "iqrf/request/DPANodeRemoveBondRequest.h"
#include "iqrf/request/DPAOSPeripheralInfoRequest.h"
#include "iqrf/request/DPAOSRestartRequest.h"

using namespace std;
using namespace Poco;

namespace BeeeOn {

class DPARequestTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DPARequestTest);
	CPPUNIT_TEST(testCreateDPARequest);
	CPPUNIT_TEST(testCreateDPABondedNodesRequest);
	CPPUNIT_TEST(testCreateDPABondNodeRequest);
	CPPUNIT_TEST(testCreateDPAUnbondNodeRequest);
	CPPUNIT_TEST(testCreateDPADiscoveryRequest);
	CPPUNIT_TEST(testClearAllBondsRequest);
	CPPUNIT_TEST(testRemoveBondRequest);
	CPPUNIT_TEST(testPeripheralInfoRequest);
	CPPUNIT_TEST(testRestartRequest);
	CPPUNIT_TEST_SUITE_END();

public:
	void testCreateDPARequest();
	void testCreateDPABondedNodesRequest();
	void testCreateDPABondNodeRequest();
	void testCreateDPAUnbondNodeRequest();
	void testCreateDPADiscoveryRequest();
	void testClearAllBondsRequest();
	void testRemoveBondRequest();
	void testPeripheralInfoRequest();
	void testRestartRequest();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DPARequestTest);

void DPARequestTest::testCreateDPARequest()
{
	const vector<uint8_t> pData = {0xcd,0xef};
	const string rawDPA = "23.01.45.67.ab.89.cd.ef";

	const DPARequest::Ptr request =
		new DPARequest(
			0x123,  // network address
			0x45,   // peripheral number
			0x67,   // peripheral command
			0x89ab, // hw PID
			pData   // peripheral data
		);

	CPPUNIT_ASSERT_EQUAL(0x0123, request->networkAddress());
	CPPUNIT_ASSERT_EQUAL(0x45, request->peripheralNumber());
	CPPUNIT_ASSERT_EQUAL(0x67, request->peripheralCommand());
	CPPUNIT_ASSERT_EQUAL(0x89ab, request->HWPID());
	CPPUNIT_ASSERT(pData == request->peripheralData());

	CPPUNIT_ASSERT_EQUAL(rawDPA, request->toDPAString());
}

/**
 * Test for creating DPA bonded nodes request and compare that
 * output with DPA string which was created by IQRF IDE using
 * the same command.
 */
void DPARequestTest::testCreateDPABondedNodesRequest()
{
	const string rawDPA = "00.00.00.02.ff.ff";
	const DPARequest::Ptr request = new DPACoordBondedNodesRequest;

	CPPUNIT_ASSERT_EQUAL(rawDPA, request->toDPAString());
}

void DPARequestTest::testCreateDPABondNodeRequest()
{
	const string rawDPA = "00.00.00.04.ff.ff.00.00";
	const DPARequest::Ptr request = new DPACoordBondNodeRequest;

	CPPUNIT_ASSERT_EQUAL(rawDPA, request->toDPAString());
}

void DPARequestTest::testCreateDPAUnbondNodeRequest()
{
	const string rawDPA = "00.00.00.05.ff.ff.01";
	const DPARequest::Ptr request = new DPACoordRemoveNodeRequest(1);

	CPPUNIT_ASSERT_EQUAL(rawDPA, request->toDPAString());
}

void DPARequestTest::testCreateDPADiscoveryRequest()
{
	const string rawDPA = "00.00.00.07.ff.ff.07.00";
	const DPARequest::Ptr request = new DPACoordDiscoveryRequest;

	CPPUNIT_ASSERT_EQUAL(rawDPA, request->toDPAString());
}

void DPARequestTest::testClearAllBondsRequest()
{
	const string rawDPA = "00.00.00.03.ff.ff";
	const DPARequest::Ptr request = new DPACoordClearAllBondsRequest;

	CPPUNIT_ASSERT_EQUAL(rawDPA, request->toDPAString());
}

void DPARequestTest::testRemoveBondRequest()
{
	const string rawDPA = "12.00.01.01.ff.ff";
	const DPARequest::Ptr request = new DPANodeRemoveBondRequest(0x12);

	CPPUNIT_ASSERT_EQUAL(rawDPA, request->toDPAString());
}

void DPARequestTest::testPeripheralInfoRequest()
{
	const string rawDPA = "12.00.02.00.ff.ff";
	const DPARequest::Ptr request = new DPAOSPeripheralInfoRequest(0x12);

	CPPUNIT_ASSERT_EQUAL(rawDPA, request->toDPAString());
}

void DPARequestTest::testRestartRequest()
{
	const string rawDPA = "12.00.02.08.ff.ff";
	const DPARequest::Ptr request = new DPAOSRestartRequest(0x12);

	CPPUNIT_ASSERT_EQUAL(rawDPA, request->toDPAString());
}

}
