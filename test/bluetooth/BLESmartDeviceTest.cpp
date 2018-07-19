#include <cppunit/extensions/HelperMacros.h>
#include <vector>

#include "bluetooth/BeeWiSmartClim.h"
#include "model/SensorData.h"
#include "net/MACAddress.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class BLESmartDeviceTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(BLESmartDeviceTest);
	CPPUNIT_TEST(testBeeWiSmartClimParseValidData);
	CPPUNIT_TEST(testBeeWiSmartClimParseTooLongMessage);
	CPPUNIT_TEST(testBeeWiSmartClimParseTooShortMessage);
	CPPUNIT_TEST_SUITE_END();
public:
	void testBeeWiSmartClimParseValidData();
	void testBeeWiSmartClimParseTooLongMessage();
	void testBeeWiSmartClimParseTooShortMessage();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BLESmartDeviceTest);

/**
 * @brief Test of parsing valid values from BeeWi SmartClim sensor.
 */
void BLESmartDeviceTest::testBeeWiSmartClimParseValidData()
{
	BeeWiSmartClim sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values1 =
		{0x05, 0x00, 0xc8, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x64};
	SensorData data1 = sensor.parseAdvertisingData(values1);
	CPPUNIT_ASSERT_EQUAL(data1[0].value(), 20.0);
	CPPUNIT_ASSERT_EQUAL(int(data1[1].value()), 60);
	CPPUNIT_ASSERT_EQUAL(int(data1[2].value()), 100);

	vector<unsigned char> values2 =
		{0x05, 0x00, 0x5e, 0x01, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x50};
	SensorData data2 = sensor.parseAdvertisingData(values2);
	CPPUNIT_ASSERT_EQUAL(data2[0].value(), 35.0);
	CPPUNIT_ASSERT_EQUAL(int(data2[1].value()), 80);
	CPPUNIT_ASSERT_EQUAL(int(data2[2].value()), 80);

	vector<unsigned char> values3 =
		{0x05, 0x00, 0xcd, 0xff, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x3c};
	SensorData data3 = sensor.parseAdvertisingData(values3);
	CPPUNIT_ASSERT_EQUAL(data3[0].value(), -5.0);
	CPPUNIT_ASSERT_EQUAL(int(data3[1].value()), 100);
	CPPUNIT_ASSERT_EQUAL(int(data3[2].value()), 60);
}

/**
 * @brief Test of parsing too long message from BeeWi SmartClim sensor.
 */
void BLESmartDeviceTest::testBeeWiSmartClimParseTooLongMessage()
{
	BeeWiSmartClim sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values =
		{0x05, 0x00, 0xcd, 0xff, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x00};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 11 B, received 12 B",
		sensor.parseAdvertisingData(values),
		ProtocolException);
}

/**
 * @brief Test of parsing too short message from BeeWi SmartClim sensor.
 */
void BLESmartDeviceTest::testBeeWiSmartClimParseTooShortMessage()
{
	BeeWiSmartClim sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values = {0x00, 0x00};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 11 B, received 2 B",
		sensor.parseAdvertisingData(values),
		ProtocolException);
}

}
