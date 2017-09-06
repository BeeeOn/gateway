#include <cmath>

#include <Poco/Timespan.h>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BetterAssert.h>

#include "model/SensorData.h"
#include "util/JSONSensorDataParser.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class JSONSensorDataParserTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(JSONSensorDataParserTest);
	CPPUNIT_TEST(testParse);
	CPPUNIT_TEST(testParseNaN);
	CPPUNIT_TEST(testParseNoValues);
	CPPUNIT_TEST_SUITE_END();
public:
	void testParse();
	void testParseNaN();
	void testParseNoValues();
};

CPPUNIT_TEST_SUITE_REGISTRATION(JSONSensorDataParserTest);

void JSONSensorDataParserTest::testParse()
{
	const string stringForm =
			R"({"device_id":"0x499602d2","timestamp":95000000000,"data":[{"module_id":5,"value":4.2},{"module_id":4,"value":0.5}]})";

	JSONSensorDataParser parser;
	SensorData data = parser.parse(stringForm);

	CPPUNIT_ASSERT_EQUAL("0x499602d2", data.deviceID().toString());
	CPPUNIT_ASSERT_EQUAL(95000000000, data.timestamp().value().epochMicroseconds());

	CPPUNIT_ASSERT_EQUAL(5, data[0].moduleID());
	CPPUNIT_ASSERT_EQUAL(4.2, data[0].value());

	CPPUNIT_ASSERT_EQUAL(4, data[1].moduleID());
	CPPUNIT_ASSERT_EQUAL(0.5, data[1].value());
}

void JSONSensorDataParserTest::testParseNaN()
{
	const string stringForm =
			R"({"device_id":"0x499602d2","timestamp":95000000000,"data":[{"module_id":5,"value":15439.15},{"module_id":4,"value":null}]})";

	JSONSensorDataParser parser;
	SensorData data = parser.parse(stringForm);

	CPPUNIT_ASSERT_EQUAL("0x499602d2", data.deviceID().toString());
	CPPUNIT_ASSERT_EQUAL(95000000000, data.timestamp().value().epochMicroseconds());

	CPPUNIT_ASSERT_EQUAL(5, data[0].moduleID());
	CPPUNIT_ASSERT_EQUAL(15439.15, data[0].value());

	CPPUNIT_ASSERT_EQUAL(4, data[1].moduleID());
	CPPUNIT_ASSERT(std::isnan(data[1].value()));
}

void JSONSensorDataParserTest::testParseNoValues()
{
	SensorData data;
	data.setDeviceID(0x499602d4);

	const string stringForm = R"({"device_id":"0x499602d4","timestamp":)"
							+ to_string(data.timestamp().value().epochMicroseconds())
							+ R"(,"data":[]})";

	JSONSensorDataParser parser;

	CPPUNIT_ASSERT(data == parser.parse(stringForm));
}

}
