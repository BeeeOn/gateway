#include <string>
#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Exception.h>

#include "cppunit/BetterAssert.h"
#include "jablotron/JablotronReport.h"

using namespace std;
using namespace Poco;

namespace BeeeOn {

class JablotronReportTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(JablotronReportTest);
	CPPUNIT_TEST(testAC88);
	CPPUNIT_TEST(testJA80L);
	CPPUNIT_TEST(testTP82N);
	CPPUNIT_TEST_SUITE_END();
public:
	void testInvalid();
	void testAC88();
	void testJA80L();
	void testTP82N();
};

CPPUNIT_TEST_SUITE_REGISTRATION(JablotronReportTest);

void JablotronReportTest::testInvalid()
{
	const JablotronReport &invalid = JablotronReport::invalid();

	CPPUNIT_ASSERT(!static_cast<bool>(invalid));
	CPPUNIT_ASSERT(!invalid);
}

void JablotronReportTest::testAC88()
{
	const JablotronReport report0 = {0xCF0000, "AC-88", "RELAY:0"};
	const JablotronReport report1 = {0xCF0000, "AC-88", "RELAY:1"};

	CPPUNIT_ASSERT(static_cast<bool>(report0));
	CPPUNIT_ASSERT(report0.has("RELAY", true));
	CPPUNIT_ASSERT(!report0.has("RELAY", false));
	CPPUNIT_ASSERT_EQUAL(0, report0.get("RELAY"));

	CPPUNIT_ASSERT(static_cast<bool>(report1));
	CPPUNIT_ASSERT(report1.has("RELAY", true));
	CPPUNIT_ASSERT_EQUAL(1, report1.get("RELAY"));
}

void JablotronReportTest::testJA80L()
{
	const JablotronReport report0 = {0x580000, "JA-80L", "BUTTON BLACKOUT:0"};
	const JablotronReport report1 = {0x580000, "JA-80L", "BUTTON BLACKOUT:1"};
	const JablotronReport report2 = {0x580000, "JA-80L", "TAMPER BLACKOUT:0"};
	const JablotronReport report3 = {0x580000, "JA-80L", "TAMPER BLACKOUT:1"};
	const JablotronReport report4 = {0x580000, "JA-80L", "BEACON BLACKOUT:0"};
	const JablotronReport report5 = {0x580000, "JA-80L", "BEACON BLACKOUT:1"};

	CPPUNIT_ASSERT(report0.has("BUTTON"));
	CPPUNIT_ASSERT(!report0.has("TAMPER"));
	CPPUNIT_ASSERT(!report0.has("BEACON"));
	CPPUNIT_ASSERT(report0.has("BLACKOUT", true));
	CPPUNIT_ASSERT(!report0.has("BLACKOUT", false));
	CPPUNIT_ASSERT_EQUAL(0, report0.get("BLACKOUT"));

	CPPUNIT_ASSERT(report1.has("BUTTON"));
	CPPUNIT_ASSERT(!report1.has("TAMPER"));
	CPPUNIT_ASSERT(!report1.has("BEACON"));
	CPPUNIT_ASSERT(report1.has("BLACKOUT", true));
	CPPUNIT_ASSERT_EQUAL(1, report1.get("BLACKOUT"));

	CPPUNIT_ASSERT(!report2.has("BUTTON"));
	CPPUNIT_ASSERT(report2.has("TAMPER"));
	CPPUNIT_ASSERT(!report2.has("BEACON"));
	CPPUNIT_ASSERT(report2.has("BLACKOUT", true));
	CPPUNIT_ASSERT_EQUAL(0, report2.get("BLACKOUT"));

	CPPUNIT_ASSERT(!report3.has("BUTTON"));
	CPPUNIT_ASSERT(report3.has("TAMPER"));
	CPPUNIT_ASSERT(!report3.has("BEACON"));
	CPPUNIT_ASSERT(report3.has("BLACKOUT", true));
	CPPUNIT_ASSERT_EQUAL(1, report3.get("BLACKOUT"));

	CPPUNIT_ASSERT(!report4.has("BUTTON"));
	CPPUNIT_ASSERT(!report4.has("TAMPER"));
	CPPUNIT_ASSERT(report4.has("BEACON"));
	CPPUNIT_ASSERT(report4.has("BLACKOUT", true));
	CPPUNIT_ASSERT_EQUAL(0, report4.get("BLACKOUT"));

	CPPUNIT_ASSERT(!report5.has("BUTTON"));
	CPPUNIT_ASSERT(!report5.has("TAMPER"));
	CPPUNIT_ASSERT(report5.has("BEACON"));
	CPPUNIT_ASSERT(report5.has("BLACKOUT", true));
	CPPUNIT_ASSERT_EQUAL(1, report5.get("BLACKOUT"));
}

void JablotronReportTest::testTP82N()
{
	static const string C = {static_cast<char>(0xb0), 'C', '\0'};
	const JablotronReport report0 = {0x240000, "TP-82N", "SET:20.5" + C + " LB:0"};
	const JablotronReport report1 = {0x240000, "TP-82N", "SET:15.8" + C + " LB:1"};
	const JablotronReport report2 = {0x240000, "TP-82N", "INT:16.4" + C + " LB:0"};
	const JablotronReport report3 = {0x240000, "TP-82N", "INT:10.0" + C + " LB:1"};
	const JablotronReport report4 = {0x240000, "TP-82N", "INT:-14.0" + C + " LB:0"};

	CPPUNIT_ASSERT(report0.has("SET", true));
	CPPUNIT_ASSERT(!report0.has("INT", true));
	CPPUNIT_ASSERT(report0.has("LB", true));
	CPPUNIT_ASSERT_EQUAL(20.5, report0.temperature("SET"));
	CPPUNIT_ASSERT_EQUAL(0, report0.get("LB"));
	CPPUNIT_ASSERT_EQUAL(100, report0.battery());

	CPPUNIT_ASSERT(report1.has("SET", true));
	CPPUNIT_ASSERT(!report1.has("INT", true));
	CPPUNIT_ASSERT(report1.has("LB", true));
	CPPUNIT_ASSERT_EQUAL(15.8, report1.temperature("SET"));
	CPPUNIT_ASSERT_EQUAL(1, report1.get("LB"));
	CPPUNIT_ASSERT_EQUAL(5, report1.battery());

	CPPUNIT_ASSERT(!report2.has("SET", true));
	CPPUNIT_ASSERT(report2.has("INT", true));
	CPPUNIT_ASSERT(report2.has("LB", true));
	CPPUNIT_ASSERT_EQUAL(16.4, report2.temperature("INT"));
	CPPUNIT_ASSERT_EQUAL(0, report2.get("LB"));
	CPPUNIT_ASSERT_EQUAL(100, report2.battery());

	CPPUNIT_ASSERT(!report3.has("SET", true));
	CPPUNIT_ASSERT(report3.has("INT", true));
	CPPUNIT_ASSERT(report3.has("LB", true));
	CPPUNIT_ASSERT_EQUAL(10.0, report3.temperature("INT"));
	CPPUNIT_ASSERT_EQUAL(1, report3.get("LB"));
	CPPUNIT_ASSERT_EQUAL(5, report3.battery());

	CPPUNIT_ASSERT(!report4.has("SET", true));
	CPPUNIT_ASSERT(report4.has("INT", true));
	CPPUNIT_ASSERT(report4.has("LB", true));
	CPPUNIT_ASSERT_EQUAL(-14.0, report4.temperature("INT"));
	CPPUNIT_ASSERT_EQUAL(0, report4.get("LB"));
	CPPUNIT_ASSERT_EQUAL(100, report4.battery());
}

}
