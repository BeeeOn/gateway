#include <string>
#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Exception.h>

#include "cppunit/BetterAssert.h"
#include "jablotron/JablotronGadget.h"

using namespace std;
using namespace Poco;

namespace BeeeOn {

class JablotronGadgetTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(JablotronGadgetTest);
	CPPUNIT_TEST(testResolve);
	CPPUNIT_TEST(testToString);
	CPPUNIT_TEST(testPrimary);
	CPPUNIT_TEST(testSecondary);
	CPPUNIT_TEST_SUITE_END();
public:
	void testResolve();
	void testToString();
	void testPrimary();
	void testSecondary();
};

CPPUNIT_TEST_SUITE_REGISTRATION(JablotronGadgetTest);

void JablotronGadgetTest::testResolve()
{
	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(13565952));
	CPPUNIT_ASSERT_EQUAL("AC-88 (sensor)", JablotronGadget::Info::resolve(13565952).name());
	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(13631487));
	CPPUNIT_ASSERT_EQUAL("AC-88 (sensor)", JablotronGadget::Info::resolve(13631487).name());

	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(5767168));
	CPPUNIT_ASSERT_EQUAL("JA-80L", JablotronGadget::Info::resolve(5767168).name());
	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(5898239));
	CPPUNIT_ASSERT_EQUAL("JA-80L", JablotronGadget::Info::resolve(5898239).name());

	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(1572864));
	CPPUNIT_ASSERT_EQUAL("JA-81M", JablotronGadget::Info::resolve(1572864).name());
	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(1835007));
	CPPUNIT_ASSERT_EQUAL("JA-81M", JablotronGadget::Info::resolve(1835007).name());

	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(8323072));
	CPPUNIT_ASSERT_EQUAL("JA-82SH", JablotronGadget::Info::resolve(8323072).name());
	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(8388607));
	CPPUNIT_ASSERT_EQUAL("JA-82SH", JablotronGadget::Info::resolve(8388607).name());

	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(1835008));
	CPPUNIT_ASSERT_EQUAL("JA-83M", JablotronGadget::Info::resolve(1835008).name());
	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(1966079));
	CPPUNIT_ASSERT_EQUAL("JA-83M", JablotronGadget::Info::resolve(1966079).name());

	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(6553600));
	CPPUNIT_ASSERT_EQUAL("JA-83P", JablotronGadget::Info::resolve(6553600).name());
	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(6684671));
	CPPUNIT_ASSERT_EQUAL("JA-83P", JablotronGadget::Info::resolve(6684671).name());

	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(7733248));
	CPPUNIT_ASSERT_EQUAL("JA-85ST", JablotronGadget::Info::resolve(7733248).name());
	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(7798783));
	CPPUNIT_ASSERT_EQUAL("JA-85ST", JablotronGadget::Info::resolve(7798783).name());

	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(8388608));
	CPPUNIT_ASSERT_EQUAL("RC-86K (dual)", JablotronGadget::Info::resolve(8388608).name());
	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(8912895));
	CPPUNIT_ASSERT_EQUAL("RC-86K (dual)", JablotronGadget::Info::resolve(8912895).name());

	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(2359296));
	CPPUNIT_ASSERT_EQUAL("TP-82N", JablotronGadget::Info::resolve(2359296).name());
	CPPUNIT_ASSERT(JablotronGadget::Info::resolve(2490367));
	CPPUNIT_ASSERT_EQUAL("TP-82N", JablotronGadget::Info::resolve(2490367).name());
}

void JablotronGadgetTest::testToString()
{
	JablotronGadget ac88   = {0, 13565952, JablotronGadget::Info::resolve(13565952)};
	JablotronGadget ja80l  = {1,  5767168, JablotronGadget::Info::resolve(5767168)};
	JablotronGadget ja81m  = {2,  1572864, JablotronGadget::Info::resolve(1572864)};
	JablotronGadget ja82sh = {3,  8323072, JablotronGadget::Info::resolve(8323072)};
	JablotronGadget ja83m  = {4,  1835008, JablotronGadget::Info::resolve(1835008)};
	JablotronGadget ja83p  = {5,  6553600, JablotronGadget::Info::resolve(6553600)};
	JablotronGadget ja85st = {6,  7733248, JablotronGadget::Info::resolve(7733248)};
	JablotronGadget rc86k  = {7,  8388608, JablotronGadget::Info::resolve(8388608)};
	JablotronGadget tp82n  = {8,  2359296, JablotronGadget::Info::resolve(2359296)};

	CPPUNIT_ASSERT_EQUAL("SLOT:00 [13565952] AC-88 (sensor)", ac88.toString());
	CPPUNIT_ASSERT_EQUAL("SLOT:01 [05767168] JA-80L",  ja80l.toString());
	CPPUNIT_ASSERT_EQUAL("SLOT:02 [01572864] JA-81M",  ja81m.toString());
	CPPUNIT_ASSERT_EQUAL("SLOT:03 [08323072] JA-82SH", ja82sh.toString());
	CPPUNIT_ASSERT_EQUAL("SLOT:04 [01835008] JA-83M",  ja83m.toString());
	CPPUNIT_ASSERT_EQUAL("SLOT:05 [06553600] JA-83P",  ja83p.toString());
	CPPUNIT_ASSERT_EQUAL("SLOT:06 [07733248] JA-85ST", ja85st.toString());
	CPPUNIT_ASSERT_EQUAL("SLOT:07 [08388608] RC-86K (dual)",  rc86k.toString());
	CPPUNIT_ASSERT_EQUAL("SLOT:08 [02359296] TP-82N",  tp82n.toString());
}

void JablotronGadgetTest::testPrimary()
{
	CPPUNIT_ASSERT_EQUAL(       0, JablotronGadget::Info::primaryAddress(0));
	CPPUNIT_ASSERT_EQUAL(13565952, JablotronGadget::Info::primaryAddress(13565952));
	CPPUNIT_ASSERT_EQUAL( 5767168, JablotronGadget::Info::primaryAddress(5767168));
	CPPUNIT_ASSERT_EQUAL( 1572864, JablotronGadget::Info::primaryAddress(1572864));
	CPPUNIT_ASSERT_EQUAL( 8323072, JablotronGadget::Info::primaryAddress(8323072));
	CPPUNIT_ASSERT_EQUAL( 1835008, JablotronGadget::Info::primaryAddress(1835008));
	CPPUNIT_ASSERT_EQUAL( 6553600, JablotronGadget::Info::primaryAddress(6553600));
	CPPUNIT_ASSERT_EQUAL( 7733248, JablotronGadget::Info::primaryAddress(7733248));
	CPPUNIT_ASSERT_EQUAL( 2359296, JablotronGadget::Info::primaryAddress(2359296));

	CPPUNIT_ASSERT_EQUAL( 8388608, JablotronGadget::Info::primaryAddress(8388608));
	CPPUNIT_ASSERT_EQUAL( 8912895, JablotronGadget::Info::primaryAddress(8912895));
	CPPUNIT_ASSERT_EQUAL( 8388608, JablotronGadget::Info::primaryAddress(9437184));
	CPPUNIT_ASSERT_EQUAL( 8912895, JablotronGadget::Info::primaryAddress(9961471));
}

void JablotronGadgetTest::testSecondary()
{
	CPPUNIT_ASSERT_EQUAL(       0, JablotronGadget::Info::secondaryAddress(0));
	CPPUNIT_ASSERT_EQUAL(13565952, JablotronGadget::Info::secondaryAddress(13565952));
	CPPUNIT_ASSERT_EQUAL( 5767168, JablotronGadget::Info::secondaryAddress(5767168));
	CPPUNIT_ASSERT_EQUAL( 1572864, JablotronGadget::Info::secondaryAddress(1572864));
	CPPUNIT_ASSERT_EQUAL( 8323072, JablotronGadget::Info::secondaryAddress(8323072));
	CPPUNIT_ASSERT_EQUAL( 1835008, JablotronGadget::Info::secondaryAddress(1835008));
	CPPUNIT_ASSERT_EQUAL( 6553600, JablotronGadget::Info::secondaryAddress(6553600));
	CPPUNIT_ASSERT_EQUAL( 7733248, JablotronGadget::Info::secondaryAddress(7733248));
	CPPUNIT_ASSERT_EQUAL( 2359296, JablotronGadget::Info::secondaryAddress(2359296));

	CPPUNIT_ASSERT_EQUAL( 9437184, JablotronGadget::Info::secondaryAddress(9437184));
	CPPUNIT_ASSERT_EQUAL( 9961471, JablotronGadget::Info::secondaryAddress(9961471));
	CPPUNIT_ASSERT_EQUAL( 9437184, JablotronGadget::Info::secondaryAddress(8388608));
	CPPUNIT_ASSERT_EQUAL( 9961471, JablotronGadget::Info::secondaryAddress(8912895));
}

}
