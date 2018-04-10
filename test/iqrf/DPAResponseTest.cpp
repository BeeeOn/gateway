#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"
#include "iqrf/DPAResponse.h"

using namespace std;
using namespace Poco;

namespace BeeeOn {

class DPAResponseTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DPAResponseTest);
	CPPUNIT_TEST(testCreateDPAResponseFromRaw);
	CPPUNIT_TEST_SUITE_END();

public:
	void testCreateDPAResponseFromRaw();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DPAResponseTest);

void DPAResponseTest::testCreateDPAResponseFromRaw()
{
	const vector<uint8_t> pData = {0x01, 0x23};
	const string rawDPA = "23.01.45.67.ab.89.cd.ef.01.23";

	const DPAResponse::Ptr response = DPAResponse::fromRaw(rawDPA);

	CPPUNIT_ASSERT_EQUAL(0x0123, response->networkAddress());
	CPPUNIT_ASSERT_EQUAL(0x45, response->peripheralNumber());
	CPPUNIT_ASSERT_EQUAL(0x67, response->peripheralCommand());
	CPPUNIT_ASSERT_EQUAL(0x89ab, response->HWPID());
	CPPUNIT_ASSERT_EQUAL(0xcd, response->errorCode());
	CPPUNIT_ASSERT_EQUAL(0xef, response->dpaValue());
	CPPUNIT_ASSERT(pData == response->peripheralData());

	CPPUNIT_ASSERT_EQUAL(rawDPA, response->toDPAString());
}

}
