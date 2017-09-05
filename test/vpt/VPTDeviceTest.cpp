#include <cppunit/extensions/HelperMacros.h>
#include <string>

#include <Poco/Exception.h>

#include "cppunit/BetterAssert.h"

#include "model/DeviceID.h"
#include "model/DevicePrefix.h"
#include "vpt/VPTDevice.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class VPTDeviceTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(VPTDeviceTest);
	CPPUNIT_TEST(testCreateSubdeviceID);
	CPPUNIT_TEST(testOmitSubdeviceFromDeviceID);
	CPPUNIT_TEST(testExtractSubdeviceFromDeviceID);
	CPPUNIT_TEST(testExtractNonce);
	CPPUNIT_TEST(testGenerateHashPassword);
	CPPUNIT_TEST_SUITE_END();

public:
	void testCreateSubdeviceID();
	void testOmitSubdeviceFromDeviceID();
	void testExtractSubdeviceFromDeviceID();
	void testExtractNonce();
	void testGenerateHashPassword();
};

CPPUNIT_TEST_SUITE_REGISTRATION(VPTDeviceTest);

/*
 * @brief Test of creating DeviceIDs for all subdevices (4 zones + boiler).
 */
void VPTDeviceTest::testCreateSubdeviceID()
{
	DeviceID realVPTid(DevicePrefix::PREFIX_VPT, 0x0000112233445566);

	// DeviceID for boiler
	CPPUNIT_ASSERT(0xa400112233445566 == VPTDevice::createSubdeviceID(0, realVPTid));
	// DeviceID for zone 1
	CPPUNIT_ASSERT(0xa401112233445566 == VPTDevice::createSubdeviceID(1, realVPTid));
	// DeviceID for zone 2
	CPPUNIT_ASSERT(0xa402112233445566 == VPTDevice::createSubdeviceID(2, realVPTid));
	// DeviceID for zone 3
	CPPUNIT_ASSERT(0xa403112233445566 == VPTDevice::createSubdeviceID(3, realVPTid));
	// DeviceID for zone 4
	CPPUNIT_ASSERT(0xa404112233445566 == VPTDevice::createSubdeviceID(4, realVPTid));
}

/*
 * @brief Test of omitting subdevice number from DeviceID. Also
 * it checks if it throws exception when the subdevice number is
 * out of range.
 */
void VPTDeviceTest::testOmitSubdeviceFromDeviceID()
{
	DeviceID boilerID(DevicePrefix::PREFIX_VPT, 0x0000112233445566);
	CPPUNIT_ASSERT(0xa400112233445566 == VPTDevice::omitSubdeviceFromDeviceID(boilerID));

	DeviceID zone1ID(DevicePrefix::PREFIX_VPT, 0x0001112233445566);
	CPPUNIT_ASSERT(0xa400112233445566 == VPTDevice::omitSubdeviceFromDeviceID(zone1ID));

	DeviceID zone2ID(DevicePrefix::PREFIX_VPT, 0x0002112233445566);
	CPPUNIT_ASSERT(0xa400112233445566 == VPTDevice::omitSubdeviceFromDeviceID(zone2ID));

	DeviceID zone3ID(DevicePrefix::PREFIX_VPT, 0x0003112233445566);
	CPPUNIT_ASSERT(0xa400112233445566 == VPTDevice::omitSubdeviceFromDeviceID(zone3ID));

	DeviceID zone4ID(DevicePrefix::PREFIX_VPT, 0x0004112233445566);
	CPPUNIT_ASSERT(0xa400112233445566 == VPTDevice::omitSubdeviceFromDeviceID(zone4ID));

	DeviceID wrong1ID(DevicePrefix::PREFIX_VPT, 0x0005112233445566);
	CPPUNIT_ASSERT_THROW_MESSAGE("invalid subdevice number 5",
		VPTDevice::omitSubdeviceFromDeviceID(wrong1ID),
		InvalidArgumentException);

	DeviceID wrong2ID(DevicePrefix::PREFIX_VPT, 0x000f112233445566);
	CPPUNIT_ASSERT_THROW_MESSAGE("invalid subdevice number 15",
		VPTDevice::omitSubdeviceFromDeviceID(wrong2ID),
		InvalidArgumentException);

	DeviceID wrong3ID(DevicePrefix::PREFIX_VPT, 0x00ff112233445566);
	CPPUNIT_ASSERT_THROW_MESSAGE("invalid subdevice number 255",
		VPTDevice::omitSubdeviceFromDeviceID(wrong3ID),
		InvalidArgumentException);
}

/*
 * @brief Test of extracting subdevice number from DeviceID. Also
 * it checks if it throws exception when the subdevice number is
 * out of range.
 */
void VPTDeviceTest::testExtractSubdeviceFromDeviceID()
{
	DeviceID boilerID(DevicePrefix::PREFIX_VPT, 0x0000112233445566);
	CPPUNIT_ASSERT(0 == VPTDevice::extractSubdeviceFromDeviceID(boilerID));

	DeviceID zone1ID(DevicePrefix::PREFIX_VPT, 0x0001112233445566);
	CPPUNIT_ASSERT(1 == VPTDevice::extractSubdeviceFromDeviceID(zone1ID));

	DeviceID zone2ID(DevicePrefix::PREFIX_VPT, 0x0002112233445566);
	CPPUNIT_ASSERT(2 == VPTDevice::extractSubdeviceFromDeviceID(zone2ID));

	DeviceID zone3ID(DevicePrefix::PREFIX_VPT, 0x0003112233445566);
	CPPUNIT_ASSERT(3 == VPTDevice::extractSubdeviceFromDeviceID(zone3ID));

	DeviceID zone4ID(DevicePrefix::PREFIX_VPT, 0x0004112233445566);
	CPPUNIT_ASSERT(4 == VPTDevice::extractSubdeviceFromDeviceID(zone4ID));

	DeviceID wrong1ID(DevicePrefix::PREFIX_VPT, 0x0005112233445566);
	CPPUNIT_ASSERT_THROW_MESSAGE("invalid subdevice number 5",
		VPTDevice::extractSubdeviceFromDeviceID(wrong1ID),
		InvalidArgumentException);

	DeviceID wrong2ID(DevicePrefix::PREFIX_VPT, 0x000f112233445566);
	CPPUNIT_ASSERT_THROW_MESSAGE("invalid subdevice number 15",
		VPTDevice::extractSubdeviceFromDeviceID(wrong2ID),
		InvalidArgumentException);

	DeviceID wrong3ID(DevicePrefix::PREFIX_VPT, 0x00ff112233445566);
	CPPUNIT_ASSERT_THROW_MESSAGE("invalid subdevice number 255",
		VPTDevice::extractSubdeviceFromDeviceID(wrong3ID),
		InvalidArgumentException);
}

/*
 * @brief Test of extracting random number from text.
 */
void VPTDeviceTest::testExtractNonce()
{
	string text1 = R"(text text text text text)"
	               R"(text text var randnum = 42 text text)"
	               R"(text text text text text)";
	CPPUNIT_ASSERT("42" == VPTDevice::extractNonce(text1));

	string text2 = R"(text text text text text\n)"
	               R"(text text text text text\n)"
	               R"(var randnum = 42\n)"
	               R"(text text text text)";
	CPPUNIT_ASSERT("42" == VPTDevice::extractNonce(text2));

	string text3 = R"(var randnum = 42 text text text text)"
	               R"(text text text text text)"
	               R"(text text text text text)";
	CPPUNIT_ASSERT("42" == VPTDevice::extractNonce(text3));
}

/*
 * @brief Test of generating sha1 hash of password + random number.
 */
void VPTDeviceTest::testGenerateHashPassword()
{
	CPPUNIT_ASSERT("3c7b3a2ffe38d977df9d6fa4455c9f8403ce374f" == VPTDevice::generateHashPassword("password", "125"));
	CPPUNIT_ASSERT("e9dee14b78ab69de5c91a0c7bc216c7652953b54" == VPTDevice::generateHashPassword("strongerpassword", "1025"));
	CPPUNIT_ASSERT("4ede3da98ce049c6cedea83fa4b159aa84a6ef1c" == VPTDevice::generateHashPassword("theultrastrongestpassword", "10025"));
}

}
