#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"

#include "iqrf/DPAIQHomeProtocol.h"

using namespace std;
using namespace Poco;

namespace BeeeOn {

class DPAIQHomeProtocolTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DPAIQHomeProtocolTest);
	CPPUNIT_TEST(testExtractModules);
	CPPUNIT_TEST(testExtractProductInfo);
	CPPUNIT_TEST(testExtractValues);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp() override;
	void testExtractModules();
	void testExtractProductInfo();
	void testExtractValues();
private:
	DPAIQHomeProtocol m_iqHomeProtocol;
};

CPPUNIT_TEST_SUITE_REGISTRATION(DPAIQHomeProtocolTest);

static const string XML_BUFFER(
	"<iqrf-iqhome-mapping>\n"
	"  <map comment='Temperature'>\n"
	"    <iqrf-iqhome id='0x01' error-value='0x8000' wide='2' "
	"      resolution='0.0625' signed='yes' />\n"
	"    <beeeon type='temperature' />\n"
	"  </map>\n"
	"  <map comment='Humidity'>\n"
	"    <iqrf-iqhome id='0x02' error-value='0x8000' wide='2' "
	"      resolution='0.0625' signed='yes' />\n"
	"    <beeeon type='humidity' />\n"
	"  </map>\n"
	"  <map comment='CO2'>\n"
	"    <iqrf-iqhome id='0x03' error-value='0x8000' wide='2' "
	"      resolution='1' signed='no' />\n"
	"    <beeeon type='co2' />\n"
	"  </map>\n"
	"</iqrf-iqhome-mapping>\n"
);

void DPAIQHomeProtocolTest::setUp()
{
	istringstream buffer;
	buffer.str(XML_BUFFER);

	m_iqHomeProtocol.loadTypesMapping(buffer);
}

/**
 * @brief Test of module extraction from a given message. The given
 * message is response from IQRF device to request with supported modules.
 */
void DPAIQHomeProtocolTest::testExtractModules()
{
	vector<uint8_t> peripheralData =
		{0x02, 0x01, 0xb4, 0x01, 0x02, 0x06, 0x02};

	const auto l = m_iqHomeProtocol.extractModules(peripheralData);
	const vector<ModuleType> modulesVec(l.begin(), l.end());

	CPPUNIT_ASSERT_EQUAL(4, l.size());
	CPPUNIT_ASSERT_EQUAL(ModuleType::Type::TYPE_TEMPERATURE, modulesVec.at(0).type());
	CPPUNIT_ASSERT_EQUAL(ModuleType::Type::TYPE_HUMIDITY, modulesVec.at(1).type());

	CPPUNIT_ASSERT_EQUAL(ModuleType::Type::TYPE_BATTERY, modulesVec.at(2).type());
	CPPUNIT_ASSERT_EQUAL(ModuleType::Type::TYPE_RSSI, modulesVec.at(3).type());
}

/**
 * @brief Test of product info extraction from a given message. The given message
 * is response from IQRF device to request with product info.
 */
void DPAIQHomeProtocolTest::testExtractProductInfo()
{
	DPAIQHomeProtocol::Ptr iqHome = new DPAIQHomeProtocol;

	const list<ModuleType> modules = {
		{ModuleType::Type::TYPE_TEMPERATURE},
		{ModuleType::Type::TYPE_HUMIDITY},
	};

	const vector<uint8_t> values = {
		0x53, 0x4e, 0x2d, 0x54,
		0x48, 0x2d, 0x30, 0x31,
		0x20, 0x20, 0x20, 0x48,
		0x31, 0x38, 0x30, 0x33
	};

	DPAProtocol::ProductInfo info = iqHome->extractProductInfo(values, 0x15AF);
	CPPUNIT_ASSERT_EQUAL("IQHome", info.vendorName);
	CPPUNIT_ASSERT_EQUAL("SN-TH-01", info.productName);

	// invalid HWPID of IQ Home
	CPPUNIT_ASSERT_THROW(
		iqHome->extractProductInfo(values, 0x0000),
		InvalidArgumentException
	);

	// invalid response size
	CPPUNIT_ASSERT_THROW(
		iqHome->extractProductInfo({}, 0x15AF),
		ProtocolException
	);
}

/**
 * @brief Test of measured value extraction from a given message. The given
 * message is response from IQRF device to request with measured values.
 */
void DPAIQHomeProtocolTest::testExtractValues()
{
	const list<ModuleType> modules = {
		{ModuleType::Type::TYPE_TEMPERATURE},
		{ModuleType::Type::TYPE_HUMIDITY},
		{ModuleType::Type::TYPE_CO2},
	};

	const vector<uint8_t> values = {
		0x02,             // status register
		0x01, 0xb7, 0x01, // valid temperature value
		0x02, 0xf5, 0x01, // valid humidity value
	};

	auto var = m_iqHomeProtocol.parseValue(modules, values);

	uint16_t moduleID = 0;

	// Temperature
	CPPUNIT_ASSERT_EQUAL(moduleID++, var.at(0).moduleID());
	CPPUNIT_ASSERT_EQUAL(27.4375, var.at(0).value());

	// Humidity
	CPPUNIT_ASSERT_EQUAL(moduleID++, var.at(1).moduleID());
	CPPUNIT_ASSERT_EQUAL(31.3125, var.at(1).value());
}

}
