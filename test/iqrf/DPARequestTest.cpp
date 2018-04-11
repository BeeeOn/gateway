#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"
#include "iqrf/DPARequest.h"
#include "iqrf/request/DPACoordBondedNodesRequest.h"

using namespace std;
using namespace Poco;

namespace BeeeOn {

class DPARequestTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DPARequestTest);
	CPPUNIT_TEST(testCreateDPARequest);
	CPPUNIT_TEST(testCreateDPABondedNodesRequest);
	CPPUNIT_TEST_SUITE_END();

public:
	void testCreateDPARequest();
	void testCreateDPABondedNodesRequest();
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

}
