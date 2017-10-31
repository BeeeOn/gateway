#include <cmath>

#include <Poco/Timespan.h>

#include <cppunit/extensions/HelperMacros.h>

#include "model/SensorData.h"
#include "util/CSVSensorDataFormatter.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class CSVSensorDataFormatterTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(CSVSensorDataFormatterTest);
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

CPPUNIT_TEST_SUITE_REGISTRATION(CSVSensorDataFormatterTest);

void CSVSensorDataFormatterTest::testFormat()
{
	SensorData data;
	data.setDeviceID(0x499602d2);
	data.insertValue(SensorValue(ModuleID(5), 4.2));
	data.insertValue(SensorValue(ModuleID(4), 0.5));

	CSVSensorDataFormatter formatter;
	const string str = formatter.format(data);

	const string timestamp = to_string(data.timestamp().value().epochTime());
	const string expected = "sensor;" + timestamp + ";0x499602d2;5;4.20;\n"
		+ "sensor;" + timestamp + ";0x499602d2;4;0.50;";

	CPPUNIT_ASSERT_EQUAL(expected, str);
}

void CSVSensorDataFormatterTest::testFormatNaN()
{
	SensorData data;
	data.setDeviceID(0x499602d3);
	data.insertValue(SensorValue(ModuleID(6), NAN));
	data.insertValue(SensorValue(ModuleID(2), 154454.2456));

	CSVSensorDataFormatter formatter;
	string str = formatter.format(data);

	const string timestamp = to_string(data.timestamp().value().epochTime());
	const string expected = "sensor;" + timestamp + ";0x499602d3;6;nan;\n"
		+ "sensor;" + timestamp + ";0x499602d3;2;154454.25;";

	CPPUNIT_ASSERT_EQUAL(expected, str);
}

void CSVSensorDataFormatterTest::testFormatINFINITY()
{
	SensorData data;
	data.setDeviceID(0x499602d3);
	data.insertValue(SensorValue(ModuleID(6), INFINITY));
	data.insertValue(SensorValue(ModuleID(2)));

	CSVSensorDataFormatter formatter;
	string str = formatter.format(data);

	const string timestamp = to_string(data.timestamp().value().epochTime());
	const string expected = "sensor;" + timestamp + ";0x499602d3;6;inf;\n"
		+ "sensor;" + timestamp + ";0x499602d3;2;nan;";

	CPPUNIT_ASSERT_EQUAL(expected, str);
}

void CSVSensorDataFormatterTest::testFormatNoValues()
{
	SensorData data;
	data.setDeviceID(0x499602d4);

	CSVSensorDataFormatter formatter;
	const string str = formatter.format(data);

	const string expected = "";

	CPPUNIT_ASSERT_EQUAL(expected, str);
}

}
