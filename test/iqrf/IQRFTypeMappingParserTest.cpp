#include <sstream>

#include <cppunit/BetterAssert.h>
#include <cppunit/extensions/HelperMacros.h>

#include "iqrf/IQRFTypeMappingParser.h"

using namespace std;
using namespace Poco;

namespace BeeeOn {

class IQRFTypeMappingParserTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(IQRFTypeMappingParserTest);
	CPPUNIT_TEST(testParse);
	CPPUNIT_TEST(testParseMissingId);
	CPPUNIT_TEST(testParseMissingErrorValue);
	CPPUNIT_TEST(testParseMissingWide);
	CPPUNIT_TEST(testParseMissingResolution);
	CPPUNIT_TEST(testParseMissingSignedFlag);
	CPPUNIT_TEST_SUITE_END();
public:
	void testParse();
	void testParseMissingId();
	void testParseMissingErrorValue();
	void testParseMissingWide();
	void testParseMissingResolution();
	void testParseMissingSignedFlag();
};

CPPUNIT_TEST_SUITE_REGISTRATION(IQRFTypeMappingParserTest);

void IQRFTypeMappingParserTest::testParse()
{
	istringstream buffer;
	buffer.str(
		"<mappings>\n"
		"  <map comment='Temperature'>\n"
		"    <iqrf id='0x01' error-value='0x8000' wide='2' "
		"      resolution='0.0625' signed='yes' />\n"
		"    <beeeon type='temperature' />\n"
		"  </map>\n"
		"</mappings>\n"
	);

	IQRFTypeMappingParser parser("mappings", "iqrf");
	const auto sequence = parser.parse(buffer);

	CPPUNIT_ASSERT_EQUAL(1, sequence.size());

	const auto temperature = sequence[0];
	CPPUNIT_ASSERT_EQUAL(0x01, temperature.first.id);
	CPPUNIT_ASSERT_EQUAL(0x8000, temperature.first.errorValue);
	CPPUNIT_ASSERT_EQUAL(2, temperature.first.wide);
	CPPUNIT_ASSERT_EQUAL(0.0625, temperature.first.resolution);
	CPPUNIT_ASSERT_EQUAL(true, temperature.first.signedFlag);
	CPPUNIT_ASSERT_EQUAL(ModuleType::Type::TYPE_TEMPERATURE, temperature.second.type());
}

void IQRFTypeMappingParserTest::testParseMissingId()
{
	istringstream buffer;
	buffer.str(
		"<mappings>\n"
		"  <map comment='Temperature'>\n"
		"    <iqrf error-value='0x8000' wide='2' "
		"      resolution='0.0625' signed='yes' />\n"
		"    <beeeon type='temperature' />\n"
		"  </map>\n"
		"</mappings>\n"
	);

	IQRFTypeMappingParser parser("mappings", "iqrf");
	CPPUNIT_ASSERT_THROW(parser.parse(buffer), SyntaxException);
}

void IQRFTypeMappingParserTest::testParseMissingErrorValue()
{
	istringstream buffer;
	buffer.str(
		"<mappings>\n"
		"  <map comment='Temperature'>\n"
		"    <iqrf id='0x01' wide='2' "
		"      resolution='0.0625' signed='yes' />\n"
		"    <beeeon type='temperature' />\n"
		"  </map>\n"
		"</mappings>\n"
	);

	IQRFTypeMappingParser parser("mappings", "iqrf");
	CPPUNIT_ASSERT_THROW(parser.parse(buffer), SyntaxException);
}

void IQRFTypeMappingParserTest::testParseMissingWide()
{
	istringstream buffer;
	buffer.str(
		"<mappings>\n"
		"  <map comment='Temperature'>\n"
		"    <iqrf id='0x01' error-value='0x8000' "
		"      resolution='0.0625' signed='yes' />\n"
		"    <beeeon type='temperature' />\n"
		"  </map>\n"
		"</mappings>\n"
	);

	IQRFTypeMappingParser parser("mappings", "iqrf");
	CPPUNIT_ASSERT_THROW(parser.parse(buffer), SyntaxException);
}

void IQRFTypeMappingParserTest::testParseMissingResolution()
{
	istringstream buffer;
	buffer.str(
		"<mappings>\n"
		"  <map comment='Temperature'>\n"
		"    <iqrf id='0x01' error-value='0x8000' wide='2' "
		"      signed='yes' />\n"
		"    <beeeon type='temperature' />\n"
		"  </map>\n"
		"</mappings>\n"
	);

	IQRFTypeMappingParser parser("mappings", "iqrf");
	CPPUNIT_ASSERT_THROW(parser.parse(buffer), SyntaxException);
}

void IQRFTypeMappingParserTest::testParseMissingSignedFlag()
{
	istringstream buffer;
	buffer.str(
		"<mappings>\n"
		"  <map comment='Temperature'>\n"
		"    <iqrf id='0x01' error-value='0x8000' wide='2' "
		"      resolution='0.0625' />\n"
		"    <beeeon type='temperature' />\n"
		"  </map>\n"
		"</mappings>\n"
	);

	IQRFTypeMappingParser parser("mappings", "iqrf");
	CPPUNIT_ASSERT_THROW(parser.parse(buffer), SyntaxException);
}

}
