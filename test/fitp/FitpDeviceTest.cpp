#include <cmath>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>

#include "cppunit/BetterAssert.h"
#include "fitp/FitpDevice.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class FitpDeviceTest : public CppUnit::TestFixture {
CPPUNIT_TEST_SUITE(FitpDeviceTest);
	CPPUNIT_TEST(testParseMessage);
	CPPUNIT_TEST(testModuleValueInvalid);
	CPPUNIT_TEST(testParseValue);
	CPPUNIT_TEST(testNegativeValue);
CPPUNIT_TEST_SUITE_END();

public:
void testParseMessage();
void testModuleValueInvalid();
void testParseValue();
void testNegativeValue();
};

CPPUNIT_TEST_SUITE_REGISTRATION(FitpDeviceTest);

/**
 * Parse message containing 4 data payloads giving:
 *
 * Temperature INNER: 30.00 °C
 * Temperature OUTER: 25.47 °C
 * Humidity: 44.53 %
 * Battery: 100 %
 */
void FitpDeviceTest::testParseMessage()
{
	Timestamp now;
	DeviceID deviceID = 0xa1000000ed000004;
	FitpDevice device(deviceID);

	const vector<uint8_t> values = {0x92, 0x01, 0x00, 0x00, 0x05, 0x00, 0x0b, 0xb8,
				0x01, 0x00, 0x00, 0x09, 0xf3, 0x02, 0x00, 0x00, 0x11, 0x65,
				0x03, 0x00, 0x00, 0x0e, 0x7c, 0x05, 0x00, 0x1e};
	SensorData sensorData = device.parseMessage(values, deviceID);

	CPPUNIT_ASSERT(now <= sensorData.timestamp());
	CPPUNIT_ASSERT_EQUAL(4, sensorData.size());

	CPPUNIT_ASSERT(sensorData.at(0).moduleID().value() == 1);      //BATTERY
	CPPUNIT_ASSERT_EQUAL(sensorData.at(0).value(), 100);

	CPPUNIT_ASSERT(sensorData.at(1).moduleID().value() == 2);      //TEMPERATURE INNER, SHT21
	CPPUNIT_ASSERT_EQUAL(sensorData.at(1).value(), 25.47);

	CPPUNIT_ASSERT(sensorData.at(2).moduleID().value() == 3);       //TEMPERATURE OUTER, Ds18b20
	CPPUNIT_ASSERT_EQUAL(sensorData.at(2).value(), 44);

	CPPUNIT_ASSERT(sensorData.at(3).moduleID().value() == 4);      //HUMIDITY, SHT21
	CPPUNIT_ASSERT_EQUAL(sensorData.at(3).value(), 37);
}

void FitpDeviceTest::testParseValue()
{
	const vector<uint8_t> values = {0x0b, 0xb8};

	double value = FitpDevice::extractValue(values);

	CPPUNIT_ASSERT(3000 == value);
}

void FitpDeviceTest::testNegativeValue()
{
	const vector<uint8_t> values = {0xff, 0xff, 0xff, 0xff};

	double value = FitpDevice::extractValue(values);

	CPPUNIT_ASSERT(-1 == value);
}

void FitpDeviceTest::testModuleValueInvalid()
{
	DeviceID deviceID = 0xa1000000ed000004;
	FitpDevice device(deviceID);

	const vector<uint8_t> values = {0x92, 0x01, 0x00, 0x00, 0x05, 0x00, 0x7f, 0xff};
	SensorData sensorData = device.parseMessage(values, deviceID);

	CPPUNIT_ASSERT(!sensorData.at(0).isValid());
	CPPUNIT_ASSERT(std::isnan(sensorData.at(0).value()));
}

}
