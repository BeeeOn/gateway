#include <cppunit/extensions/HelperMacros.h>
#include <vector>

#include "bluetooth/BeeWiSmartClim.h"
#include "bluetooth/BeeWiSmartDoor.h"
#include "bluetooth/BeeWiSmartMotion.h"
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
	CPPUNIT_TEST(testBeeWiSmartMotionParseValidData);
	CPPUNIT_TEST(testBeeWiSmartMotionParseTooLongMessage);
	CPPUNIT_TEST(testBeeWiSmartMotionParseTooShortMessage);
	CPPUNIT_TEST(testBeeWiSmartDoorParseValidData);
	CPPUNIT_TEST(testBeeWiSmartDoorParseTooLongMessage);
	CPPUNIT_TEST(testBeeWiSmartDoorParseTooShortMessage);
	CPPUNIT_TEST_SUITE_END();
public:
	void testBeeWiSmartClimParseValidData();
	void testBeeWiSmartClimParseTooLongMessage();
	void testBeeWiSmartClimParseTooShortMessage();
	void testBeeWiSmartMotionParseValidData();
	void testBeeWiSmartMotionParseTooLongMessage();
	void testBeeWiSmartMotionParseTooShortMessage();
	void testBeeWiSmartDoorParseValidData();
	void testBeeWiSmartDoorParseTooLongMessage();
	void testBeeWiSmartDoorParseTooShortMessage();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BLESmartDeviceTest);

class TestableBeeWiSmartMotion: public BeeWiSmartMotion {
public:
	TestableBeeWiSmartMotion(const MACAddress& address, const Timespan& timeout):
		BeeWiSmartMotion(address, timeout)
	{
	}
};

class TestableBeeWiSmartDoor: public BeeWiSmartDoor {
public:
	TestableBeeWiSmartDoor(const MACAddress& address, const Timespan& timeout):
		BeeWiSmartDoor(address, timeout)
	{
	}
};

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

/**
 * @brief Test of parsing valid values from BeeWi Smart Motion sesnor.
 */
void BLESmartDeviceTest::testBeeWiSmartMotionParseValidData()
{
	TestableBeeWiSmartMotion sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values1 =
		{0x06, 0x08, 0x01, 0x00, 0x64};
	SensorData data1 = sensor.parseAdvertisingData(values1);
	CPPUNIT_ASSERT_EQUAL(int(data1[0].value()), 1);
	CPPUNIT_ASSERT_EQUAL(int(data1[1].value()), 100);

	vector<unsigned char> values2 =
		{0x06, 0x08, 0x00, 0x00, 0x05};
	SensorData data2 = sensor.parseAdvertisingData(values2);
	CPPUNIT_ASSERT_EQUAL(int(data2[0].value()), 0);
	CPPUNIT_ASSERT_EQUAL(int(data2[1].value()), 5);
}

/**
 * @brief Test of parsing too long message from BeeWi Smart Motion sensor.
 */
void BLESmartDeviceTest::testBeeWiSmartMotionParseTooLongMessage()
{
	TestableBeeWiSmartMotion sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values =
		{0x06, 0x08, 0x00, 0x00, 0x64, 0x00};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 5 B, received 6 B",
		sensor.parseAdvertisingData(values),
		ProtocolException);
}

/**
 * @brief Test of parsing too short message from BeeWi Smart Motion sensor.
 */
void BLESmartDeviceTest::testBeeWiSmartMotionParseTooShortMessage()
{
	TestableBeeWiSmartMotion sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values = {0x00, 0x00};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 5 B, received 2 B",
		sensor.parseAdvertisingData(values),
		ProtocolException);
}

/**
 * @brief Test of parsing valid values from BeeWi Smart Door sesnor.
 */
void BLESmartDeviceTest::testBeeWiSmartDoorParseValidData()
{
	TestableBeeWiSmartDoor sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values1 =
		{0x07, 0x08, 0x01, 0x00, 0x64};
	SensorData data1 = sensor.parseAdvertisingData(values1);
	CPPUNIT_ASSERT_EQUAL(int(data1[0].value()), 1);
	CPPUNIT_ASSERT_EQUAL(int(data1[1].value()), 100);

	vector<unsigned char> values2 =
		{0x07, 0x08, 0x00, 0x00, 0x05};
	SensorData data2 = sensor.parseAdvertisingData(values2);
	CPPUNIT_ASSERT_EQUAL(int(data2[0].value()), 0);
	CPPUNIT_ASSERT_EQUAL(int(data2[1].value()), 5);
}

/**
 * @brief Test of parsing too long message from BeeWi Smart Door sensor.
 */
void BLESmartDeviceTest::testBeeWiSmartDoorParseTooLongMessage()
{
	TestableBeeWiSmartDoor sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values =
		{0x07, 0x08, 0x00, 0x00, 0x64, 0x00};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 5 B, received 6 B",
		sensor.parseAdvertisingData(values),
		ProtocolException);
}

/**
 * @brief Test of parsing too short message from BeeWi Smart Door sensor.
 */
void BLESmartDeviceTest::testBeeWiSmartDoorParseTooShortMessage()
{
	TestableBeeWiSmartDoor sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values = {0x00, 0x00};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 5 B, received 2 B",
		sensor.parseAdvertisingData(values),
		ProtocolException);
}

}
