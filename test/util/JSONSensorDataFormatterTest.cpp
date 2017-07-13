#include <cmath>

#include <Poco/Timespan.h>

#include <cppunit/extensions/HelperMacros.h>

#include "model/SensorData.h"
#include "util/JSONSensorDataFormatter.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class JSONSensorDataFormatterTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(JSONSensorDataFormatterTest);
	CPPUNIT_TEST(testFormat);
	CPPUNIT_TEST(testFormatNaN);
	CPPUNIT_TEST(testFormatINFINITY);
	CPPUNIT_TEST(testFormatNoValues);
	CPPUNIT_TEST_SUITE_END();
public:
	void testFormat();
	void testFormatNaN();
	void testFormatINFINITY();
	void testFormatNoValues();
};

CPPUNIT_TEST_SUITE_REGISTRATION(JSONSensorDataFormatterTest);

void JSONSensorDataFormatterTest::testFormat()
{
	SensorData data;
	data.setDeviceID(0x499602d2);
	data.insertValue(SensorValue(ModuleID(5), 4.2));
	data.insertValue(SensorValue(ModuleID(4), 0.5));

	JSONSensorDataFormatter formatter;
	const string str = formatter.format(data);

	Timestamp now;
	const string expected = R"({"device_id":"0x499602d2","timestamp":)"
		+ to_string(now.epochTime())
		+ R"(,"data":[{"module_id":5,"value":4.2},{"module_id":4,"value":0.5}]})";

	CPPUNIT_ASSERT_EQUAL(str, expected);
}

void JSONSensorDataFormatterTest::testFormatNaN()
{
	SensorData data;
	data.setDeviceID(0x499602d3);
	data.insertValue(SensorValue(ModuleID(6), NAN));
	data.insertValue(SensorValue(ModuleID(2), 154454.2456));

	JSONSensorDataFormatter formatter;
	string str = formatter.format(data);

	Timestamp now;
	const string expected = R"({"device_id":"0x499602d3","timestamp":)"
		+ to_string(now.epochTime())
		+ R"(,"data":[{"module_id":6,"value":null},{"module_id":2,"value":154454}]})";

	CPPUNIT_ASSERT_EQUAL(str, expected);
}

void JSONSensorDataFormatterTest::testFormatINFINITY()
{
	SensorData data;
	data.setDeviceID(0x499602d3);
	data.insertValue(SensorValue(ModuleID(6), INFINITY));
	data.insertValue(SensorValue(ModuleID(2)));

	JSONSensorDataFormatter formatter;
	string str = formatter.format(data);

	Timestamp now;
	const string expected = R"({"device_id":"0x499602d3","timestamp":)"
		+ to_string(now.epochTime())
		+ R"(,"data":[{"module_id":6,"value":null},{"module_id":2}]})";

	CPPUNIT_ASSERT_EQUAL(str, expected);
}

void JSONSensorDataFormatterTest::testFormatNoValues()
{
	SensorData data;
	data.setDeviceID(0x499602d4);

	JSONSensorDataFormatter formatter;
	const string str = formatter.format(data);

	Timestamp now;
	const string expected = R"({"device_id":"0x499602d4","timestamp":)"
		+ to_string(now.epochTime())
		+ R"(,"data":[]})";

	CPPUNIT_ASSERT_EQUAL(str, expected);
}

}
