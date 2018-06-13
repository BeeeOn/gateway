#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"
#include "iqrf/DPAResponse.h"
#include "iqrf/response/DPACoordBondNodeResponse.h"
#include "iqrf/response/DPACoordBondedNodesResponse.h"
#include "iqrf/response/DPACoordRemoveNodeResponse.h"
#include "iqrf/response/DPAOSPeripheralInfoResponse.h"

using namespace std;
using namespace Poco;

namespace BeeeOn {

class DPAResponseTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DPAResponseTest);
	CPPUNIT_TEST(testCreateDPAResponseFromRaw);
	CPPUNIT_TEST(testParseBondedNodesResponse);
	CPPUNIT_TEST(testParseBondNodeResponse);
	CPPUNIT_TEST(testParseRemoveNode);
	CPPUNIT_TEST(testParsePeripheralInfoResponse);
	CPPUNIT_TEST_SUITE_END();

public:
	void testCreateDPAResponseFromRaw();
	void testParseBondedNodesResponse();
	void testParseBondNodeResponse();
	void testParseRemoveNode();
	void testParsePeripheralInfoResponse();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DPAResponseTest);

void DPAResponseTest::testCreateDPAResponseFromRaw()
{
	const vector<uint8_t> pData = {0x01, 0x23};
	const string rawDPA = "23.01.45.67.ab.89.cd.ef.01.23";

	const DPAResponse::Ptr response = DPAResponse::fromRaw(rawDPA);

	CPPUNIT_ASSERT_EQUAL(0x0123, response->networkAddress());
	CPPUNIT_ASSERT_EQUAL(0x45, response->peripheralNumber());
	CPPUNIT_ASSERT_EQUAL(0x67, response->peripheralCommand());
	CPPUNIT_ASSERT_EQUAL(0x89ab, response->HWPID());
	CPPUNIT_ASSERT_EQUAL(0xcd, response->errorCode());
	CPPUNIT_ASSERT_EQUAL(0xef, response->dpaValue());
	CPPUNIT_ASSERT(pData == response->peripheralData());

	CPPUNIT_ASSERT_EQUAL(rawDPA, response->toDPAString());
}

/**
 * Test for parse DPA bonded nodes response and compare
 * that output with DPA string which was created by IQRF IDE
 * using the same command.
 *
 * DPA message contains bit index with nodes.
 */
void DPAResponseTest::testParseBondedNodesResponse()
{
	// bonded nodes
	set<uint8_t> bondedNodes;
	bondedNodes.emplace(1);
	bondedNodes.emplace(2);
	bondedNodes.emplace(3);

	DPAMessage::Ptr response = DPAResponse::fromRaw(
		"00.00.00.82.ff.ff.00.00." // dpa response header
		"0E.00.00.00.00.00.00.00."
		"00.00.00.00.00.00.00.00."
		"00.00.00.00.00.00.00.00."
		"00.00.00.00.00.00.00.00");

	const auto nodes = response.cast<DPACoordBondedNodesResponse>()->decodeNodeBonded();
	CPPUNIT_ASSERT_EQUAL(3, nodes.size());

	CPPUNIT_ASSERT(bondedNodes == nodes);
}

void DPAResponseTest::testParseBondNodeResponse()
{
	const DPAMessage::Ptr response = DPAResponse::fromRaw(
		"00.00.00.84.ff.ff.00.00." // dpa response header
		"03.09");

	CPPUNIT_ASSERT_EQUAL(3, response.cast<DPACoordBondNodeResponse>()->count());
	CPPUNIT_ASSERT_EQUAL(9, response.cast<DPACoordBondNodeResponse>()->bondedNetworkAddress());
}

void DPAResponseTest::testParseRemoveNode()
{
	const DPAMessage::Ptr response = DPAResponse::fromRaw(
		"00.00.00.85.ff.ff.00.00." // dpa response header
		"07");

	CPPUNIT_ASSERT_EQUAL(7, response.cast<DPACoordRemoveNodeResponse>()->count());
}

void DPAResponseTest::testParsePeripheralInfoResponse()
{
	const DPAMessage::Ptr response = DPAResponse::fromRaw(
		"00.00.02.80.ff.ff.00.00." // dpa response header
		"E4.57.00.81.42.B4.B8.08.5C.1F.00.85");

	// response with minimal valid values (rssi and supply voltage)
	const DPAMessage::Ptr resMin = DPAResponse::fromRaw(
		"00.00.02.80.ff.ff.00.00." // dpa response header
		"E4.57.00.81.42.B4.B8.08.0C.00.00.85");

	// response with maximal valid values (rssi and supply voltage)
	const DPAMessage::Ptr resMax = DPAResponse::fromRaw(
		"00.00.02.80.ff.ff.00.00." // dpa response header
		"E4.57.00.81.42.B4.B8.08.8D.3B.00.85");

	// invalid value of rssi and supply voltage
	const DPAMessage::Ptr resInvalid = DPAResponse::fromRaw(
		"00.00.02.80.ff.ff.00.00." // dpa response header
		"E4.57.00.81.42.B4.B8.08.00.FF.00.85");

	const auto peripheralInfo = response.cast<DPAOSPeripheralInfoResponse>();
	const auto peripheralInfoMin = resMin.cast<DPAOSPeripheralInfoResponse>();
	const auto peripheralInfoMax = resMax.cast<DPAOSPeripheralInfoResponse>();
	const auto peripheralInfoInvalid = resInvalid.cast<DPAOSPeripheralInfoResponse>();

	CPPUNIT_ASSERT_EQUAL(0x810057E4, peripheralInfo->mid());

	CPPUNIT_ASSERT_EQUAL(int8_t(-38), peripheralInfo->rssi());
	CPPUNIT_ASSERT_EQUAL(int8_t(-118), peripheralInfoMin->rssi());
	CPPUNIT_ASSERT_EQUAL(int8_t(11), peripheralInfoMax->rssi());

	CPPUNIT_ASSERT_EQUAL(2.72, peripheralInfo->supplyVoltage());
	CPPUNIT_ASSERT_EQUAL(2.06, round(peripheralInfoMin->supplyVoltage() * 100.0) / 100.0);
	CPPUNIT_ASSERT_EQUAL(3.84, round(peripheralInfoMax->supplyVoltage() * 100.0) / 100.0);

	CPPUNIT_ASSERT_EQUAL(52.54, round(peripheralInfo->percentageSupplyVoltage() * 100.0) / 100.0);
	CPPUNIT_ASSERT_EQUAL(0, peripheralInfoMin->percentageSupplyVoltage());
	CPPUNIT_ASSERT_EQUAL(100, peripheralInfoMax->percentageSupplyVoltage());

	CPPUNIT_ASSERT_THROW(peripheralInfoInvalid->rssi(), RangeException);
	CPPUNIT_ASSERT_THROW(peripheralInfoInvalid->supplyVoltage(), RangeException);
	CPPUNIT_ASSERT_THROW(peripheralInfoInvalid->percentageSupplyVoltage(), RangeException);
}

}
