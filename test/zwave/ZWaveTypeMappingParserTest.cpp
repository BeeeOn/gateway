#include <sstream>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BetterAssert.h>

#include <Poco/Exception.h>

#include "zwave/ZWaveTypeMappingParser.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class ZWaveTypeMappingParserTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ZWaveTypeMappingParserTest);
	CPPUNIT_TEST(testParse);
	CPPUNIT_TEST(testParseMissingBeeeOnType);
	CPPUNIT_TEST(testParseMissingCommandClass);
	CPPUNIT_TEST(testParseMissingIndex);
	CPPUNIT_TEST_SUITE_END();
public:
	void testParse();
	void testParseMissingBeeeOnType();
	void testParseMissingCommandClass();
	void testParseMissingIndex();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ZWaveTypeMappingParserTest);

void ZWaveTypeMappingParserTest::testParse()
{
	istringstream buffer;
	buffer.str(
		"<mappings>\n"
		"  <map comment='first type'>\n"
		"    <z-wave command-class='32' index='0' />\n"
		"    <beeeon type='temperature' />\n"
		"  </map>\n"
		"  <map comment='first type'>\n"
		"    <z-wave command-class='49' index='1' />\n"
		"    <beeeon type='humidity' />\n"
		"  </map>\n"
		"</mappings>\n"
	);

	ZWaveTypeMappingParser parser;
	const auto sequence = parser.parse(buffer);

	CPPUNIT_ASSERT_EQUAL(2, sequence.size());

	const auto temperature = sequence[0];
	CPPUNIT_ASSERT_EQUAL(32, temperature.first.first);
	CPPUNIT_ASSERT_EQUAL(0, temperature.first.second);
	CPPUNIT_ASSERT_EQUAL(ModuleType::Type::TYPE_TEMPERATURE, temperature.second.type());

	const auto humidity = sequence[1];
	CPPUNIT_ASSERT_EQUAL(49, humidity.first.first);
	CPPUNIT_ASSERT_EQUAL(1, humidity.first.second);
	CPPUNIT_ASSERT_EQUAL(ModuleType::Type::TYPE_HUMIDITY, humidity.second.type());
}

void ZWaveTypeMappingParserTest::testParseMissingBeeeOnType()
{
	istringstream buffer;
	buffer.str(
		"<mappings>\n"
		"  <map comment='first type'>\n"
		"    <z-wave command-class='32' index='0' />\n"
		"    <beeeon type='temperature' />\n"
		"  </map>\n"
		"  <map comment='first type'>\n"
		"    <z-wave command-class='49' index='1' />\n"
		"    <beeeon />\n"
		"  </map>\n"
		"</mappings>\n"
	);

	ZWaveTypeMappingParser parser;
	CPPUNIT_ASSERT_THROW(parser.parse(buffer), SyntaxException);
}

void ZWaveTypeMappingParserTest::testParseMissingCommandClass()
{
	istringstream buffer;
	buffer.str(
		"<mappings>\n"
		"  <map comment='first type'>\n"
		"    <z-wave command-class='32' index='0' />\n"
		"    <beeeon type='temperature' />\n"
		"  </map>\n"
		"  <map comment='first type'>\n"
		"    <z-wave index='1' />\n"
		"    <beeeon type='humidity' />\n"
		"  </map>\n"
		"</mappings>\n"
	);

	ZWaveTypeMappingParser parser;
	CPPUNIT_ASSERT_THROW(parser.parse(buffer), SyntaxException);
}

void ZWaveTypeMappingParserTest::testParseMissingIndex()
{
	istringstream buffer;
	buffer.str(
		"<mappings>\n"
		"  <map comment='first type'>\n"
		"    <z-wave command-class='32' index='0' />\n"
		"    <beeeon type='motion' />\n"
		"  </map>\n"
		"  <map comment='first type'>\n"
		"    <z-wave command-class='49' />\n"
		"    <beeeon type='ultraviolet' />\n"
		"  </map>\n"
		"</mappings>\n"
	);

	ZWaveTypeMappingParser parser;
	const auto sequence = parser.parse(buffer);

	CPPUNIT_ASSERT_EQUAL(2, sequence.size());

	const auto motion = sequence[0];
	CPPUNIT_ASSERT_EQUAL(32, motion.first.first);
	CPPUNIT_ASSERT_EQUAL(0, motion.first.second);
	CPPUNIT_ASSERT_EQUAL(ModuleType::Type::TYPE_MOTION, motion.second.type());

	const auto uv = sequence[1];
	CPPUNIT_ASSERT_EQUAL(49, uv.first.first);
	CPPUNIT_ASSERT_EQUAL(0, uv.first.second);
	CPPUNIT_ASSERT_EQUAL(ModuleType::Type::TYPE_ULTRAVIOLET, uv.second.type());
}

}
