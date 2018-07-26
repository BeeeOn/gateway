#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Exception.h>

#include "util/ColorBrightness.h"

using namespace Poco;

namespace BeeeOn {

class ColorBrightnessTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ColorBrightnessTest);
	CPPUNIT_TEST(testCreation);
	CPPUNIT_TEST(testModifyBrightness);
	CPPUNIT_TEST(testModifyColor);
	CPPUNIT_TEST_SUITE_END();
public:
	void testCreation();
	void testModifyBrightness();
	void testModifyColor();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ColorBrightnessTest);

/**
 * @brief Test of creating ColorBrightness and counting of brightness.
 */
void ColorBrightnessTest::testCreation()
{
	ColorBrightness colorBrightness1(0xff, 0xff, 0xff, 0xff);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness1.red()), 0xff);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness1.green()), 0xff);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness1.blue()), 0xff);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness1.brightness()), 100);

	ColorBrightness colorBrightness2(0x7f, 0x00, 0x00, 0xff);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness2.red()), 0x7f);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness2.green()), 0x00);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness2.blue()), 0x00);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness2.brightness()), 50);

	ColorBrightness colorBrightness3(0x09, 0x00, 0x09, 0x60);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness3.red()), 0x09);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness3.green()), 0x00);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness3.blue()), 0x09);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness3.brightness()), 9);

	ColorBrightness colorBrightness4(0x00, 0x56, 0x00, 0x60);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness4.red()), 0x00);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness4.green()), 0x56);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness4.blue()), 0x00);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness4.brightness()), 90);

	CPPUNIT_ASSERT_THROW_MESSAGE(
		"red component(255) could not be bigger then 96",
		ColorBrightness(0xff, 0x00, 0x00, 0x60),
		IllegalStateException);
}

/**
 * @brief Test of modifing RGB components by brightness.
 */
void ColorBrightnessTest::testModifyBrightness()
{
	ColorBrightness colorBrightness(0xff, 0xff, 0xff, 0xff);

	colorBrightness.setBrightness(50);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.red()), 0x80);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.green()), 0x80);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.blue()), 0x80);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.brightness()), 50);

	colorBrightness.setBrightness(25);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.red()), 0x40);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.green()), 0x40);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.blue()), 0x40);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.brightness()), 25);

	colorBrightness.setBrightness(100);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.red()), 0xff);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.green()), 0xff);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.blue()), 0xff);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.brightness()), 100);

	colorBrightness.setBrightness(75);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.red()), 0xbf);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.green()), 0xbf);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.blue()), 0xbf);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.brightness()), 75);

	CPPUNIT_ASSERT_THROW_MESSAGE(
		"brightness could not be greater than 100",
		colorBrightness.setBrightness(150),
		IllegalStateException);
}

/**
 * @brief Test of modifing RGB components and counting of brightness.
 */
void ColorBrightnessTest::testModifyColor()
{
	ColorBrightness colorBrightness(0x60, 0x60, 0x60, 0x60);

	colorBrightness.setColor(0x30, 0x30, 0x30);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.red()), 0x30);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.green()), 0x30);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.blue()), 0x30);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.brightness()), 50);

	colorBrightness.setColor(0x59, 0x00, 0x12);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.red()), 0x59);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.green()), 0x00);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.blue()), 0x12);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.brightness()), 93);

	colorBrightness.setColor(0x12, 0x35, 0x48);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.red()), 0x12);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.green()), 0x35);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.blue()), 0x48);
	CPPUNIT_ASSERT_EQUAL(int(colorBrightness.brightness()), 75);

	CPPUNIT_ASSERT_THROW_MESSAGE(
		"green component(255) could not be bigger then 96",
		colorBrightness.setColor(0x00, 0xff, 0x00),
		IllegalStateException);

	CPPUNIT_ASSERT_THROW_MESSAGE(
		"blue component(97) could not be bigger then 96",
		colorBrightness.setColor(0x60, 0x60, 0x61),
		IllegalStateException);
}

}
