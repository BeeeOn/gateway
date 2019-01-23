#include <cppunit/extensions/HelperMacros.h>
#include <vector>

#include "bluetooth/BeeWiSmartClim.h"
#include "bluetooth/BeeWiSmartDoor.h"
#include "bluetooth/BeeWiSmartLite.h"
#include "bluetooth/BeeWiSmartMotion.h"
#include "bluetooth/BeeWiSmartWatt.h"
#include "bluetooth/RevogiSmartCandle.h"
#include "bluetooth/RevogiSmartLite.h"
#include "bluetooth/RevogiSmartPlug.h"
#include "cppunit/BetterAssert.h"
#include "model/RefreshTime.h"
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
	CPPUNIT_TEST(testBeeWiSmartWattParseValidData);
	CPPUNIT_TEST(testBeeWiSmartWattParseTooLongMessage);
	CPPUNIT_TEST(testBeeWiSmartWattParseTooShortMessage);
	CPPUNIT_TEST(testBeeWiSmartLiteParseValidData);
	CPPUNIT_TEST(testBeeWiSmartLiteParseTooLongMessage);
	CPPUNIT_TEST(testBeeWiSmartLiteParseTooShortMessage);
	CPPUNIT_TEST(testConvertBrigthnessBeeWiSmartLite);
	CPPUNIT_TEST(testConvertColorTemptBeeWiSmartLite);
	CPPUNIT_TEST(testConvertBrigthnessRevogiSmartLite);
	CPPUNIT_TEST(testConvertColorTemptRevogiSmartLite);
	CPPUNIT_TEST(testRevogiSmartLiteParseValidData);
	CPPUNIT_TEST(testRevogiSmartLiteParseTooLongMessage);
	CPPUNIT_TEST(testRevogiSmartLiteParseTooShortMessage);
	CPPUNIT_TEST(testRevogiSmartCandleParseValidData);
	CPPUNIT_TEST(testRevogiSmartCandleParseTooLongMessage);
	CPPUNIT_TEST(testRevogiSmartCandleParseTooShortMessage);
	CPPUNIT_TEST(testRevogiSmartPlugParseValidData);
	CPPUNIT_TEST(testRevogiSmartPlugParseTooLongMessage);
	CPPUNIT_TEST(testRevogiSmartPlugParseTooShortMessage);
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
	void testBeeWiSmartWattParseValidData();
	void testBeeWiSmartWattParseTooLongMessage();
	void testBeeWiSmartWattParseTooShortMessage();
	void testBeeWiSmartLiteParseValidData();
	void testBeeWiSmartLiteParseTooLongMessage();
	void testBeeWiSmartLiteParseTooShortMessage();
	void testConvertBrigthnessBeeWiSmartLite();
	void testConvertColorTemptBeeWiSmartLite();
	void testConvertBrigthnessRevogiSmartLite();
	void testConvertColorTemptRevogiSmartLite();
	void testRevogiSmartLiteParseValidData();
	void testRevogiSmartLiteParseTooLongMessage();
	void testRevogiSmartLiteParseTooShortMessage();
	void testRevogiSmartCandleParseValidData();
	void testRevogiSmartCandleParseTooLongMessage();
	void testRevogiSmartCandleParseTooShortMessage();
	void testRevogiSmartPlugParseValidData();
	void testRevogiSmartPlugParseTooLongMessage();
	void testRevogiSmartPlugParseTooShortMessage();
};

CPPUNIT_TEST_SUITE_REGISTRATION(BLESmartDeviceTest);

class TestableBeeWiSmartMotion: public BeeWiSmartMotion {
public:
	TestableBeeWiSmartMotion(const MACAddress& address, const Timespan& timeout):
		BeeWiSmartMotion(address, timeout, RefreshTime::NONE, {})
	{
	}
};

class TestableBeeWiSmartDoor: public BeeWiSmartDoor {
public:
	TestableBeeWiSmartDoor(const MACAddress& address, const Timespan& timeout):
		BeeWiSmartDoor(address, timeout, RefreshTime::NONE, {})
	{
	}
};

class TestableBeeWiSmartWatt: public BeeWiSmartWatt {
public:
	TestableBeeWiSmartWatt(const MACAddress& address, const Timespan& timeout):
		BeeWiSmartWatt(address, timeout, RefreshTime::NONE, {})
	{
	}
	using BeeWiSmartWatt::parseValues;
};

class TestableBeeWiSmartLite : public BeeWiSmartLite {
public:
	TestableBeeWiSmartLite(const MACAddress& address, const Timespan& timeout):
		BeeWiSmartLite(address, timeout, RefreshTime::NONE, {})
	{
	}
	using BeeWiSmartLite::brightnessToPercentages;
	using BeeWiSmartLite::brightnessFromPercentages;
	using BeeWiSmartLite::colorTempToKelvins;
	using BeeWiSmartLite::colorTempFromKelvins;
};

class TestableRevogiSmartLite : public RevogiSmartLite {
public:
	TestableRevogiSmartLite(const MACAddress& address, const Timespan& timeout):
		RevogiSmartLite(address, timeout, RefreshTime::NONE, {})
	{
	}
	using RevogiSmartLite::brightnessFromPercents;
	using RevogiSmartLite::brightnessToPercents;
	using RevogiSmartLite::colorTempFromKelvins;
	using RevogiSmartLite::colorTempToKelvins;
	using RevogiSmartLite::parseValues;
};

class TestableRevogiSmartCandle : public RevogiSmartCandle {
public:
	TestableRevogiSmartCandle(
			const string& name,
			const MACAddress& address,
			const Timespan& timeout):
		RevogiSmartCandle(name, address, timeout, RefreshTime::NONE, {})
	{
	}
	using RevogiSmartCandle::parseValues;
};

class TestableRevogiSmartPlug: public RevogiSmartPlug {
public:
	TestableRevogiSmartPlug(const MACAddress& address, const Timespan& timeout):
		RevogiSmartPlug(address, timeout, RefreshTime::NONE, {})
	{
	}
	using RevogiSmartPlug::parseValues;
};

/**
 * @brief Test of parsing valid values from BeeWi SmartClim sensor.
 */
void BLESmartDeviceTest::testBeeWiSmartClimParseValidData()
{
	BeeWiSmartClim sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0, RefreshTime::NONE, {});

	vector<unsigned char> values1 =
		{0x05, 0x00, 0xc8, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x64};
	SensorData data1 = sensor.parseAdvertisingData(values1);
	CPPUNIT_ASSERT_EQUAL(data1[0].value(), 20.0);
	CPPUNIT_ASSERT_EQUAL(data1[1].value(), 60);
	CPPUNIT_ASSERT_EQUAL(data1[2].value(), 100);

	vector<unsigned char> values2 =
		{0x05, 0x00, 0x5e, 0x01, 0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x50};
	SensorData data2 = sensor.parseAdvertisingData(values2);
	CPPUNIT_ASSERT_EQUAL(data2[0].value(), 35.0);
	CPPUNIT_ASSERT_EQUAL(data2[1].value(), 80);
	CPPUNIT_ASSERT_EQUAL(data2[2].value(), 80);

	vector<unsigned char> values3 =
		{0x05, 0x00, 0xcd, 0xff, 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x3c};
	SensorData data3 = sensor.parseAdvertisingData(values3);
	CPPUNIT_ASSERT_EQUAL(data3[0].value(), -5.0);
	CPPUNIT_ASSERT_EQUAL(data3[1].value(), 100);
	CPPUNIT_ASSERT_EQUAL(data3[2].value(), 60);
}

/**
 * @brief Test of parsing too long message from BeeWi SmartClim sensor.
 */
void BLESmartDeviceTest::testBeeWiSmartClimParseTooLongMessage()
{
	BeeWiSmartClim sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0, RefreshTime::NONE, {});

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
	BeeWiSmartClim sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0, RefreshTime::NONE, {});

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
	CPPUNIT_ASSERT_EQUAL(data1[0].value(), 1);
	CPPUNIT_ASSERT_EQUAL(data1[1].value(), 100);

	vector<unsigned char> values2 =
		{0x06, 0x08, 0x00, 0x00, 0x05};
	SensorData data2 = sensor.parseAdvertisingData(values2);
	CPPUNIT_ASSERT_EQUAL(data2[0].value(), 0);
	CPPUNIT_ASSERT_EQUAL(data2[1].value(), 5);
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
	CPPUNIT_ASSERT_EQUAL(data1[0].value(), 1);
	CPPUNIT_ASSERT_EQUAL(data1[1].value(), 100);

	vector<unsigned char> values2 =
		{0x07, 0x08, 0x00, 0x00, 0x05};
	SensorData data2 = sensor.parseAdvertisingData(values2);
	CPPUNIT_ASSERT_EQUAL(data2[0].value(), 0);
	CPPUNIT_ASSERT_EQUAL(data2[1].value(), 5);
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

/**
 * @brief Test of parsing valid values from BeeWi Smart Watt.
 */
void BLESmartDeviceTest::testBeeWiSmartWattParseValidData()
{
	TestableBeeWiSmartWatt sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values1 =
		{0x00, 0x10, 0x00, 0xf1, 0x02, 0x00, 0x32};
	SensorData data1 = sensor.parseValues(values1);
	CPPUNIT_ASSERT_EQUAL(data1[0].value(), 0);
	CPPUNIT_ASSERT_EQUAL(data1[1].value(), 1.6);
	CPPUNIT_ASSERT_EQUAL(data1[2].value(), 241);
	CPPUNIT_ASSERT_EQUAL(data1[3].value(), 0.002);
	CPPUNIT_ASSERT_EQUAL(data1[4].value(), 50);

	vector<unsigned char> values2 =
		{0x01, 0x10, 0x01, 0xf0, 0x02, 0x01, 0x31};
	SensorData data2 = sensor.parseValues(values2);
	CPPUNIT_ASSERT_EQUAL(data2[0].value(), 1);
	CPPUNIT_ASSERT_EQUAL(data2[1].value(), 27.2);
	CPPUNIT_ASSERT_EQUAL(data2[2].value(), 240);
	CPPUNIT_ASSERT_EQUAL(data2[3].value(), 0.258);
	CPPUNIT_ASSERT_EQUAL(data2[4].value(), 49);

	vector<unsigned char> values5 =
		{0x0a, 0x03, 0x00, 0x0a, 0x00, 0x0d, 0x10, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00};
	SensorData data5 = sensor.parseAdvertisingData(values5);
	CPPUNIT_ASSERT_EQUAL(data5[0].value(), 0);
	CPPUNIT_ASSERT_EQUAL(data5[1].value(), 1.6);

	vector<unsigned char> values6 =
		{0x0a, 0x03, 0x01, 0x0a, 0x00, 0x0d, 0x10, 0x01, 0x0e, 0x00, 0x00, 0x00, 0x00};
	SensorData data6 = sensor.parseAdvertisingData(values6);
	CPPUNIT_ASSERT_EQUAL(data6[0].value(), 1);
	CPPUNIT_ASSERT_EQUAL(data6[1].value(), 27.2);
}

/**
 * @brief Test of parsing too long message from BeeWi Smart Watt.
 */
void BLESmartDeviceTest::testBeeWiSmartWattParseTooLongMessage()
{
	TestableBeeWiSmartWatt sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values1 =
		{0x01, 0x10, 0x01, 0xf0, 0x02, 0x01, 0x31, 0x00};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 7 B, received 8 B",
		sensor.parseValues(values1),
		ProtocolException);

	vector<unsigned char> values2 =
		{0x0a, 0x03, 0x00, 0x0a, 0x00, 0x0d, 0x10, 0x00, 0x0e, 0x00, 0x00, 0x00 , 0x00, 0x00};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 13 B, received 14 B",
		sensor.parseAdvertisingData(values2),
		ProtocolException);
}

/**
 * @brief Test of parsing too short message from BeeWi Smart Watt.
 */
void BLESmartDeviceTest::testBeeWiSmartWattParseTooShortMessage()
{
	TestableBeeWiSmartWatt sensor(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values1 = {0x00, 0x00};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 7 B, received 2 B",
		sensor.parseValues(values1),
		ProtocolException);

	vector<unsigned char> values2 = {0x00, 0x00};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 13 B, received 2 B",
		sensor.parseAdvertisingData(values2),
		ProtocolException);
}

/**
 * @brief Test of parsing valid values from BeeWi SmartLite bulb.
 */
void BLESmartDeviceTest::testBeeWiSmartLiteParseValidData()
{
	TestableBeeWiSmartLite light(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values1 =
		{0x06, 0x03, 0x01, 0x08, 0x22, 0x00, 0x00, 0xff};
	SensorData data1 = light.parseAdvertisingData(values1);
	CPPUNIT_ASSERT_EQUAL(data1[0].value(), 1);
	CPPUNIT_ASSERT_EQUAL(data1[1].value(), 0);
	CPPUNIT_ASSERT_EQUAL(data1[2].value(), 6000);
	CPPUNIT_ASSERT_EQUAL(data1[3].value(), 255);

	vector<unsigned char> values2 =
		{0x06, 0x03, 0x00, 0x08, 0xbb, 0xff, 0xff, 0x00};
	SensorData data2 = light.parseAdvertisingData(values2);
	CPPUNIT_ASSERT_EQUAL(data2[0].value(), 0);
	CPPUNIT_ASSERT_EQUAL(data2[1].value(), 100);
	CPPUNIT_ASSERT_EQUAL(data2[2].value(), 3000);
	CPPUNIT_ASSERT_EQUAL(data2[3].value(), 16776960);
}

/**
 * @brief Test of parsing too long message from BeeWi Smart Lite.
 */
void BLESmartDeviceTest::testBeeWiSmartLiteParseTooLongMessage()
{
	TestableBeeWiSmartLite light(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values =
		{0x06, 0x03, 0x00, 0x08, 0xbb, 0xff, 0xff, 0x00, 0x00};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 8 B, received 9 B",
		light.parseAdvertisingData(values),
		ProtocolException);
}

/**
 * @brief Test of parsing too short message from BeeWi Smart Lite.
 */
void BLESmartDeviceTest::testBeeWiSmartLiteParseTooShortMessage()
{
	TestableBeeWiSmartLite light(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values =
		{0x00, 0xbb};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 8 B, received 2 B",
		light.parseAdvertisingData(values),
		ProtocolException);
}

/**
 * @brief Test of converting brigthness value from BeeWi values to
 * BeeeOn values and back.
 */
void BLESmartDeviceTest::testConvertBrigthnessBeeWiSmartLite()
{
	TestableBeeWiSmartLite light(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	CPPUNIT_ASSERT_EQUAL(light.brightnessFromPercentages(100), 11);
	CPPUNIT_ASSERT_EQUAL(light.brightnessFromPercentages(80), 9);
	CPPUNIT_ASSERT_EQUAL(light.brightnessFromPercentages(65), 8);
	CPPUNIT_ASSERT_EQUAL(light.brightnessFromPercentages(60), 7);
	CPPUNIT_ASSERT_EQUAL(light.brightnessFromPercentages(20), 4);
	CPPUNIT_ASSERT_EQUAL(light.brightnessFromPercentages(0), 2);

	CPPUNIT_ASSERT_THROW_MESSAGE(
		"percents are out of range",
		light.brightnessFromPercentages(120),
		IllegalStateException);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"percents are out of range",
		light.brightnessFromPercentages(-20),
		IllegalStateException);

	CPPUNIT_ASSERT_EQUAL(light.brightnessToPercentages(11), 100U);
	CPPUNIT_ASSERT_EQUAL(light.brightnessToPercentages(9), 78U);
	CPPUNIT_ASSERT_EQUAL(light.brightnessToPercentages(8), 67U);
	CPPUNIT_ASSERT_EQUAL(light.brightnessToPercentages(7), 56U);
	CPPUNIT_ASSERT_EQUAL(light.brightnessToPercentages(4), 22U);
	CPPUNIT_ASSERT_EQUAL(light.brightnessToPercentages(2), 0U);

	CPPUNIT_ASSERT_THROW_MESSAGE(
		"value is out of range",
		light.brightnessToPercentages(12),
		IllegalStateException);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"value is out of range",
		light.brightnessToPercentages(1),
		IllegalStateException);
}

/**
 * @brief Test of converting color temperature value from BeeWi values to
 * BeeeOn values and back.
 */
void BLESmartDeviceTest::testConvertColorTemptBeeWiSmartLite()
{
	TestableBeeWiSmartLite light(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(25000), 2);
	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(6000), 2);
	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(5400), 4);
	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(4950), 5);
	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(4800), 6);
	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(3600), 9);
	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(3000), 11);
	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(2000), 11);

	CPPUNIT_ASSERT_THROW_MESSAGE(
		"color temperature is out of range",
		light.colorTempFromKelvins(28000),
		IllegalStateException);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"color temperature is out of range",
		light.colorTempFromKelvins(1000),
		IllegalStateException);

	CPPUNIT_ASSERT_EQUAL(light.colorTempToKelvins(11), 3000U);
	CPPUNIT_ASSERT_EQUAL(light.colorTempToKelvins(9), 3667U);
	CPPUNIT_ASSERT_EQUAL(light.colorTempToKelvins(8), 4000U);
	CPPUNIT_ASSERT_EQUAL(light.colorTempToKelvins(7), 4333U);
	CPPUNIT_ASSERT_EQUAL(light.colorTempToKelvins(4), 5333U);
	CPPUNIT_ASSERT_EQUAL(light.colorTempToKelvins(2), 6000U);
	CPPUNIT_ASSERT_EQUAL(light.colorTempToKelvins(0), 0U);

	CPPUNIT_ASSERT_THROW_MESSAGE(
		"value is out of range",
		light.colorTempToKelvins(12),
		IllegalStateException);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"value is out of range",
		light.colorTempToKelvins(1),
		IllegalStateException);
}

/**
 * @brief Test of converting birgthness value from BeeeOn values to
 * Revogi values and back.
 */
void BLESmartDeviceTest::testConvertBrigthnessRevogiSmartLite()
{
	TestableRevogiSmartLite light(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	CPPUNIT_ASSERT_EQUAL(light.brightnessFromPercents(100), 200);
	CPPUNIT_ASSERT_EQUAL(light.brightnessFromPercents(80), 160);
	CPPUNIT_ASSERT_EQUAL(light.brightnessFromPercents(50), 100);
	CPPUNIT_ASSERT_EQUAL(light.brightnessFromPercents(25), 50);
	CPPUNIT_ASSERT_EQUAL(light.brightnessFromPercents(10), 20);
	CPPUNIT_ASSERT_EQUAL(light.brightnessFromPercents(0), 0);

	CPPUNIT_ASSERT_THROW_MESSAGE(
		"percents are out of range",
		light.brightnessFromPercents(120),
		InvalidArgumentException);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"percents are out of range",
		light.brightnessFromPercents(-20),
		InvalidArgumentException);

	CPPUNIT_ASSERT_EQUAL(light.brightnessToPercents(200), 100);
	CPPUNIT_ASSERT_EQUAL(light.brightnessToPercents(160), 80);
	CPPUNIT_ASSERT_EQUAL(light.brightnessToPercents(100), 50);
	CPPUNIT_ASSERT_EQUAL(light.brightnessToPercents(50), 25);
	CPPUNIT_ASSERT_EQUAL(light.brightnessToPercents(20), 10);
	CPPUNIT_ASSERT_EQUAL(light.brightnessToPercents(0), 0);

	CPPUNIT_ASSERT_THROW_MESSAGE(
		"value are out of range",
		light.brightnessToPercents(300),
		InvalidArgumentException);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"value are out of range",
		light.brightnessToPercents(-20),
		InvalidArgumentException);
}

/**
 * @brief Test of converting color temperature value from BeeeOn values to
 * Revogi values and back.
 */
void BLESmartDeviceTest::testConvertColorTemptRevogiSmartLite()
{
	TestableRevogiSmartLite light(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(25000), 200);
	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(6500), 200);
	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(5740), 160);
	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(4600), 100);
	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(3650), 50);
	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(3080), 20);
	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(2700), 0);
	CPPUNIT_ASSERT_EQUAL(light.colorTempFromKelvins(2000), 0);

	CPPUNIT_ASSERT_THROW_MESSAGE(
		"color temperature is out of range",
		light.colorTempFromKelvins(28000),
		InvalidArgumentException);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"color temperature is out of range",
		light.colorTempFromKelvins(1000),
		InvalidArgumentException);

	CPPUNIT_ASSERT_EQUAL(light.colorTempToKelvins(200), 6500);
	CPPUNIT_ASSERT_EQUAL(light.colorTempToKelvins(160), 5740);
	CPPUNIT_ASSERT_EQUAL(light.colorTempToKelvins(100), 4600);
	CPPUNIT_ASSERT_EQUAL(light.colorTempToKelvins(50), 3650);
	CPPUNIT_ASSERT_EQUAL(light.colorTempToKelvins(20), 3080);
	CPPUNIT_ASSERT_EQUAL(light.colorTempToKelvins(0), 2700);

	CPPUNIT_ASSERT_THROW_MESSAGE(
		"value is out of range",
		light.colorTempToKelvins(300),
		InvalidArgumentException);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"value is out of range",
		light.colorTempToKelvins(-20),
		InvalidArgumentException);
}

/**
 * @brief Test of parsing valid values from Revogi Smart Lite.
 */
void BLESmartDeviceTest::testRevogiSmartLiteParseValidData()
{
	TestableRevogiSmartLite light(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values1 =
		{0x0f, 0x0e, 0x04, 0x00, 0xff, 0xff, 0xff, 0xc8, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x2e, 0xff, 0xff};
	SensorData data1 = light.parseValues(values1);
	CPPUNIT_ASSERT_EQUAL(data1[0].value(), 1);
	CPPUNIT_ASSERT_EQUAL(data1[1].value(), 100);
	CPPUNIT_ASSERT_EQUAL(data1[2].value(), 0);
	CPPUNIT_ASSERT_EQUAL(data1[3].value(), 16777215);

	vector<unsigned char> values2 =
		{0x0f, 0x0e, 0x04, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x2e, 0xff, 0xff};
	SensorData data2 = light.parseValues(values2);
	CPPUNIT_ASSERT_EQUAL(data2[0].value(), 0);
	CPPUNIT_ASSERT_EQUAL(data2[1].value(), 0);
	CPPUNIT_ASSERT_EQUAL(data2[2].value(), 0);
	CPPUNIT_ASSERT_EQUAL(data2[3].value(), 255);

	vector<unsigned char> values3 =
		{0x0f, 0x0e, 0x04, 0x00, 0xfc, 0xc8, 0xfc, 0x64, 0xc8, 0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x2e, 0xff, 0xff};
	SensorData data3 = light.parseValues(values3);
	CPPUNIT_ASSERT_EQUAL(data3[0].value(), 1);
	CPPUNIT_ASSERT_EQUAL(data3[1].value(), 50);
	CPPUNIT_ASSERT_EQUAL(data3[2].value(), 6500);
	CPPUNIT_ASSERT_EQUAL(data3[3].value(), 0);
}

/**
 * @brief Test of parsing too long message from Revogi Smart Lite.
 */
void BLESmartDeviceTest::testRevogiSmartLiteParseTooLongMessage()
{
	TestableRevogiSmartLite light(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values =
		{0x0f, 0x0e, 0x04, 0x00, 0xff, 0x00, 0xff, 0xc8, 0x00, 0x00,
		0x32, 0x30, 0x00, 0x00, 0x00, 0x00, 0x2e, 0xff, 0xff};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 18 B, received 19 B",
		light.parseValues(values),
		ProtocolException);
}

/**
 * @brief Test of parsing too short message from Revogi Smart Lite.
 */
void BLESmartDeviceTest::testRevogiSmartLiteParseTooShortMessage()
{
	TestableRevogiSmartLite light(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values =
		{0x00, 0xbb};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 18 B, received 2 B",
		light.parseValues(values),
		ProtocolException);
}

/**
 * @brief Test of parsing valid values from Revogi Smart Candle.
 */
void BLESmartDeviceTest::testRevogiSmartCandleParseValidData()
{
	TestableRevogiSmartCandle light("Delite-ED33", MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values1 =
		{0x0f, 0x0e, 0x04, 0x00, 0xff, 0xff, 0xff, 0xc8, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x2e, 0xff, 0xff};
	SensorData data1 = light.parseValues(values1);
	CPPUNIT_ASSERT_EQUAL(data1[0].value(), 1);
	CPPUNIT_ASSERT_EQUAL(data1[1].value(), 100);
	CPPUNIT_ASSERT_EQUAL(data1[2].value(), 16777215);

	vector<unsigned char> values2 =
		{0x0f, 0x0e, 0x04, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x2e, 0xff, 0xff};
	SensorData data2 = light.parseValues(values2);
	CPPUNIT_ASSERT_EQUAL(data2[0].value(), 0);
	CPPUNIT_ASSERT_EQUAL(data2[1].value(), 0);
	CPPUNIT_ASSERT_EQUAL(data2[2].value(), 255);
}

/**
 * @brief Test of parsing too long message from Revogi Smart Candle.
 */
void BLESmartDeviceTest::testRevogiSmartCandleParseTooLongMessage()
{
	TestableRevogiSmartCandle light("Delite-ED33", MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values =
		{0x0f, 0x0e, 0x04, 0x00, 0xff, 0x00, 0xff, 0xc8, 0x00, 0x00,
		0x32, 0x30, 0x00, 0x00, 0x00, 0x00, 0x2e, 0xff, 0xff};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 18 B, received 19 B",
		light.parseValues(values),
		ProtocolException);
}

/**
 * @brief Test of parsing too short message from Revogi Smart Candle.
 */
void BLESmartDeviceTest::testRevogiSmartCandleParseTooShortMessage()
{
	TestableRevogiSmartCandle light("Delite-ED33", MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values =
		{0x00, 0xbb};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 18 B, received 2 B",
		light.parseValues(values),
		ProtocolException);
}

/**
 * @brief Test of parsing valid values from Revogi Smart Plug.
 */
void BLESmartDeviceTest::testRevogiSmartPlugParseValidData()
{
	TestableRevogiSmartPlug plug(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values1 =
		{0x0f, 0x0f, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0xff,
		0xea, 0x00, 0x60, 0x32, 0x00, 0x0a, 0x2c, 0xff, 0xff};
	SensorData data1 = plug.parseValues(values1);
	CPPUNIT_ASSERT_EQUAL(data1[0].value(), 1);
	CPPUNIT_ASSERT_EQUAL(data1[1].value(), 0.255);
	CPPUNIT_ASSERT_EQUAL(data1[2].value(), 234);
	CPPUNIT_ASSERT_EQUAL(data1[3].value(), 0.096);
	CPPUNIT_ASSERT_EQUAL(data1[4].value(), 50);

	vector<unsigned char> values2 =
		{0x0f, 0x0f, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0xff, 0xff,
		0xeb, 0x01, 0x60, 0x31, 0x00, 0x0a, 0x2c, 0xff, 0xff};
	SensorData data2 = plug.parseValues(values2);
	CPPUNIT_ASSERT_EQUAL(data2[0].value(), 1);
	CPPUNIT_ASSERT_EQUAL(data2[1].value(), 65.535);
	CPPUNIT_ASSERT_EQUAL(data2[2].value(), 235);
	CPPUNIT_ASSERT_EQUAL(data2[3].value(), 0.352);
	CPPUNIT_ASSERT_EQUAL(data2[4].value(), 49);
}

/**
 * @brief Test of parsing too long message from Revogi Smart Plug.
 */
void BLESmartDeviceTest::testRevogiSmartPlugParseTooLongMessage()
{
	TestableRevogiSmartPlug plug(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values =
		{0x0f, 0x0f, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x5a, 0x56,
		0xea, 0x10, 0x05, 0x32, 0x00, 0x0a, 0x2c, 0xff, 0xff, 0xff};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 19 B, received 20 B",
		plug.parseValues(values),
		ProtocolException);
}

/**
 * @brief Test of parsing too short message from Revogi Smart Plug.
 */
void BLESmartDeviceTest::testRevogiSmartPlugParseTooShortMessage()
{
	TestableRevogiSmartPlug plug(MACAddress::parse("FF:FF:FF:FF:FF:FF"), 0);

	vector<unsigned char> values =
		{0x00, 0xbb};
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"expected 19 B, received 2 B",
		plug.parseValues(values),
		ProtocolException);
}

}
