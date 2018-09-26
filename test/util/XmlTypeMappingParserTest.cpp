#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Exception.h>
#include <Poco/String.h>
#include <Poco/DOM/NamedNodeMap.h>

#include "cppunit/BetterAssert.h"
#include "util/XmlTypeMappingParser.h"

using namespace std;
using namespace Poco;
using namespace Poco::XML;

namespace BeeeOn {

class XmlTypeMappingParserTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(XmlTypeMappingParserTest);
	CPPUNIT_TEST(testParseOneMappingGroup);
	CPPUNIT_TEST(testParseManyMappingGroups);
	CPPUNIT_TEST(testParseMissingGroupName);
	CPPUNIT_TEST(testParseMissingBeeeOnType);
	CPPUNIT_TEST_SUITE_END();
public:
	void testParseOneMappingGroup();
	void testParseManyMappingGroups();
	void testParseMissingGroupName();
	void testParseMissingBeeeOnType();
};

CPPUNIT_TEST_SUITE_REGISTRATION(XmlTypeMappingParserTest);

class TestableTypeMappingParser : public XmlTypeMappingParser<string> {
public:
	TestableTypeMappingParser(
			const string &mappingGroup,
			const string &techNode):
		XmlTypeMappingParser<string>(
			mappingGroup, techNode, Loggable::forClass(typeid(*this))),
		m_techNode(techNode)
	{
	}

	string parseTechType(const Poco::XML::Node &node) override
	{
		const Node *idNode = node.attributes()->getNamedItem("id");
		if (idNode == nullptr)
			throw SyntaxException("missing attribute id on element " + m_techNode);

		return trim(idNode->getNodeValue());
	}

	string techTypeRepr(const string &type) override
	{
		return type;
	}

private:
	string m_techNode;
};

void XmlTypeMappingParserTest::testParseOneMappingGroup()
{
	istringstream buffer;
	buffer.str(
		"<test-mapping>\n"
		"  <map comment='Temperature'>\n"
		"    <iqrf id='0x01' />\n"
		"    <beeeon type='temperature' />\n"
		"  </map>\n"
		"</test-mapping>\n"
	);

	TestableTypeMappingParser parser("test-mapping", "iqrf");
	const auto sequence = parser.parse(buffer);

	CPPUNIT_ASSERT_EQUAL(1, sequence.size());

	const auto temperature = sequence[0];
	CPPUNIT_ASSERT_EQUAL("0x01", temperature.first);
	CPPUNIT_ASSERT_EQUAL(ModuleType::Type::TYPE_TEMPERATURE, temperature.second.type());
}

void XmlTypeMappingParserTest::testParseMissingGroupName()
{
	istringstream buffer;
	buffer.str(
		"<test-mapping>\n"
		"  <map comment='Temperature'>\n"
		"    <iqrf id='0x01' />\n"
		"    <beeeon type='temperature' />\n"
		"  </map>\n"
		"</test-mapping>\n"
	);

	TestableTypeMappingParser parser("unknown-mapping", "iqrf");
	const auto sequence = parser.parse(buffer);

	CPPUNIT_ASSERT_EQUAL(0, sequence.size());
}

void XmlTypeMappingParserTest::testParseManyMappingGroups()
{
	const string mapping(
		"<types-mapping>\n"
		"  <test-mapping>\n"
		"    <map comment='Temperature'>\n"
		"      <iqrf id='0x01' />\n"
		"      <beeeon type='temperature' />\n"
		"    </map>\n"
		"  </test-mapping>\n"
		"  <test2-mapping>\n"
		"    <map comment='Humidity'>\n"
		"      <z-wave id='0x02' />\n"
		"      <beeeon type='humidity' />\n"
		"    </map>\n"
		"  </test2-mapping>\n"
		"</types-mapping>\n"
	);

	// first mapping group
	istringstream buffer;
	buffer.str(mapping);

	TestableTypeMappingParser parser("test-mapping", "iqrf");
	const auto sequence = parser.parse(buffer);

	CPPUNIT_ASSERT_EQUAL(1, sequence.size());

	const auto temperature = sequence[0];
	CPPUNIT_ASSERT_EQUAL("0x01", temperature.first);
	CPPUNIT_ASSERT_EQUAL(ModuleType::Type::TYPE_TEMPERATURE, temperature.second.type());

	// second mapping group
	istringstream buffer2;
	buffer2.str(mapping);

	TestableTypeMappingParser parser2("test2-mapping", "z-wave");
	const auto sequence2 = parser2.parse(buffer2);

	CPPUNIT_ASSERT_EQUAL(1, sequence2.size());

	const auto humidity = sequence2[0];
	CPPUNIT_ASSERT_EQUAL("0x02", humidity.first);
	CPPUNIT_ASSERT_EQUAL(ModuleType::Type::TYPE_HUMIDITY, humidity.second.type());
}

void XmlTypeMappingParserTest::testParseMissingBeeeOnType()
{
	istringstream buffer;
	buffer.str(
		"<test-mapping>\n"
		"  <map comment='Temperature'>\n"
		"    <iqrf id='0x01' />\n"
		"    <beeeon />\n"
		"  </map>\n"
		"</test-mapping>\n"
	);

	TestableTypeMappingParser parser("test-mapping", "iqrf");
	CPPUNIT_ASSERT_THROW(parser.parse(buffer), SyntaxException);
}

}
