#include <cppunit/extensions/HelperMacros.h>
#include <vector>

#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>

#include "model/SensorData.h"
#include "vektiva/VektivaDeviceManager.h"
#include "vektiva/VektivaSmarwi.h"
#include "vektiva/VektivaSmarwiStatus.h"
#include "cppunit/BetterAssert.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class VektivaSmarwiTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(VektivaSmarwiTest);
	CPPUNIT_TEST(testParseValidStatusMessage);
	CPPUNIT_TEST(testParseInvalidMessageTopic);
	CPPUNIT_TEST(testParseInvalidStatusMessage);
	CPPUNIT_TEST(testBuildMqttMessage);
	CPPUNIT_TEST_SUITE_END();
public:
	class VektivaDeviceManagerTest : public  VektivaDeviceManager {
	public:
		bool testIsTopicValid(const string &topic, const string &lastSegment);
	};
	void testParseValidStatusMessage();
	void testParseInvalidStatusMessage();
	void testParseInvalidMessageTopic();
	void testBuildMqttMessage();
protected:
	void vektivaSmarwiStatusAssertEqual(
		const VektivaSmarwiStatus &status1,
		const VektivaSmarwiStatus &status2);
};

	CPPUNIT_TEST_SUITE_REGISTRATION(VektivaSmarwiTest);

bool VektivaSmarwiTest::VektivaDeviceManagerTest::testIsTopicValid(
	const string &topic,
	const string &lastSegment)
{
	return isTopicValid(topic, lastSegment);
}

/**
 * @brief Test of parsing valid status response.
 */
void VektivaSmarwiTest::testParseValidStatusMessage()
{
	string msg =
		"t:swr\n"
		"s:250\n"
		"e:0\n"
		"ok:1\n"
		"ro:0\n"
		"pos:o\n"
		"fix:1\n"
		"a:-98\n"
		"fw:3.4.1-15-g3d0f\n"
		"mem:23960\n"
		"up:1106507\n"
		"ip:268446218\n"
		"cid:xsismi01\n"
		"rssi:-56\n"
		"time:1554165683\n"
		"wm:1\n"
		"wp:3\n"
		"wst:3";

	auto ipValue = (uint32_t) NumberParser::parse("268446218");
	struct in_addr addr = {ipValue};
	Net::IPAddress ipAddress(inet_ntoa(addr));
	VektivaSmarwiStatus correctStatus(250, 0, 1, 0, true, 1, ipAddress, -56);
	auto parsedStatus = VektivaSmarwi::parseStatusResponse(msg);

	vektivaSmarwiStatusAssertEqual(parsedStatus, correctStatus);

	string multipleNewLineCharsMsg =
		"t:swr\n\n\n"
		"s:250\n"
		"e:0\n"
		"ok:1\n"
		"ro:0\n"
		"pos:o\n"
		"fix:1\n"
		"a:-98\n"
		"fw:3.4.1-15-g3d0f\n"
		"mem:23960\n"
		"up:1106507\n"
		"ip:268446218\n"
		"cid:xsismi01\n"
		"rssi:-56\n"
		"time:1554165683\n"
		"wm:1\n"
		"wp:3\n"
		"wst:3\n\n\n\n";

	auto parsedStatus2 = VektivaSmarwi::parseStatusResponse(msg);
	vektivaSmarwiStatusAssertEqual(parsedStatus2, correctStatus);
}

/**
 * @brief Test of parsing invalid status messages.
 */
void VektivaSmarwiTest::testParseInvalidStatusMessage()
{
	string emptyMsg;
	CPPUNIT_ASSERT_THROW(VektivaSmarwi::parseStatusResponse(emptyMsg),
		SyntaxException);

	string incompleteMsg = "t:swr\n";
	CPPUNIT_ASSERT_THROW(VektivaSmarwi::parseStatusResponse(incompleteMsg),
		SyntaxException);

	string noNumbersMsg =
		"t:testswr\n"
		"s:test250\n"
		"e:test0\n"
		"ok:test1\n"
		"ro:test0\n"
		"pos:testo\n"
		"fix:test1\n"
		"a:test-98\n"
		"fw:test3.4.1-15-g3d0f\n"
		"mem:test23960\n"
		"up:test1106507\n"
		"ip:test268446218\n"
		"cid:testxsismi01\n"
		"rssi:test-56\n"
		"time:test1554165683\n"
		"wm:test1\n"
		"wp:test3\n"
		"wst:test3";

	CPPUNIT_ASSERT_THROW(VektivaSmarwi::parseStatusResponse(noNumbersMsg),
		SyntaxException);

	string multipleSemiColonsMsg =
		"t:swr\n"
		"s:250:e:0\n"
		"ok:1\n"
		"ro:0\n"
		"pos:o\n"
		"fix:1\n"
		"a:-98\n"
		"fw:3.4.1-15-g3d0f\n"
		"mem:23960\n"
		"up:1106507\n"
		"ip:268446218\n"
		"cid:xsismi01\n"
		"rssi:-56\n"
		"time:1554165683\n"
		"wm:1\n"
		"wp:3\n"
		"wst:3\n";

	CPPUNIT_ASSERT_THROW(VektivaSmarwi::parseStatusResponse(
		multipleSemiColonsMsg), SyntaxException);
}

void VektivaSmarwiTest::testParseInvalidMessageTopic()
{
	VektivaDeviceManagerTest vektivaDeviceManager;
	bool validity;

	string correctTopic = "ion/dowarogxby/%abcdefabcdef/status";
	validity = vektivaDeviceManager.testIsTopicValid(correctTopic, "status");
	CPPUNIT_ASSERT_EQUAL(true, validity);

	// Correct topic with backslash
	string correctWithBackslash = "ion/some\\thing/%abcdefabcdef/status";
	validity = vektivaDeviceManager.testIsTopicValid(correctWithBackslash,
		"status");
	CPPUNIT_ASSERT_EQUAL(true, validity);

	// Correct topic with asterisks
	string correctWithAsterisks = "ion/** ** ***/%abcdefabcdef/status";
	validity = vektivaDeviceManager.testIsTopicValid(correctWithAsterisks,
		"status");
	CPPUNIT_ASSERT_EQUAL(true, validity);

	// Topic without the last segment matching
	string notMatchingLastSegment = "ion/dowarogxby/%abcdefabcdef/online";
	validity = vektivaDeviceManager.testIsTopicValid(notMatchingLastSegment,
		"status");
	CPPUNIT_ASSERT_EQUAL(false, validity);

	// Topic without last segment
	string noLastSegment = "ion/dowarogxby/%abcdefabcdef/";
	validity = vektivaDeviceManager.testIsTopicValid(noLastSegment, "status");
	CPPUNIT_ASSERT_EQUAL(false, validity);

	// Topic without remoteID
	string noRemoteID = "ion//%abcdefabcdef/status";
	validity = vektivaDeviceManager.testIsTopicValid(noRemoteID, "status");
	CPPUNIT_ASSERT_EQUAL(false, validity);

	// Topic with an extra segment in the remote ID
	string extraSegment = "ion/a/a/%abcdefabcdef/status";
	validity = vektivaDeviceManager.testIsTopicValid(extraSegment, "status");
	CPPUNIT_ASSERT_EQUAL(false, validity);

	// Topic with an extra slash in the remote ID
	string extraSlash = "ion/something//%abcdefabcdef/status";
	validity = vektivaDeviceManager.testIsTopicValid(extraSlash, "status");
	CPPUNIT_ASSERT_EQUAL(false, validity);

	// Topic with a sharp in the remote ID
	string sharpInRemoteID = "ion/some#thing/%abcdefabcdef/status";
	validity = vektivaDeviceManager.testIsTopicValid(sharpInRemoteID, "status");
	CPPUNIT_ASSERT_EQUAL(false, validity);

	// Topic with a plus in the remote ID
	string plusInRemoteID = "ion/some+thing/%abcdefabcdef/status";
	validity = vektivaDeviceManager.testIsTopicValid(plusInRemoteID, "status");
	CPPUNIT_ASSERT_EQUAL(false, validity);

	// One char topic
	string oneChar = ".";
	validity = vektivaDeviceManager.testIsTopicValid(oneChar, "status");
	CPPUNIT_ASSERT_EQUAL(false, validity);

	// Empty topic
	string empty;
	validity = vektivaDeviceManager.testIsTopicValid(empty, "status");
	CPPUNIT_ASSERT_EQUAL(false, validity);

	// Topic with only slashes instead of remote ID
	string slashes = "ion///%abcdefabcdef/status";
	validity = vektivaDeviceManager.testIsTopicValid(slashes, "status");
	CPPUNIT_ASSERT_EQUAL(false, validity);
}

/**
* @brief Test of creating a correct MQTT message
*/
void VektivaSmarwiTest::testBuildMqttMessage()
{
	string macAddress = "aabbccddeeff";
	string remoteID = "dowarogxby";
	string command = "open";
	auto message = VektivaSmarwi::buildMqttMessage(remoteID, macAddress,
		command);

	string correctTopic = "ion/" + remoteID + "/%" + macAddress
		+ "/cmd";
	CPPUNIT_ASSERT_EQUAL(message.topic(), correctTopic);

	string correctMessage = "open";
	CPPUNIT_ASSERT_EQUAL(message.message(), correctMessage);
}

void VektivaSmarwiTest::vektivaSmarwiStatusAssertEqual(
	const VektivaSmarwiStatus &status1,
	const VektivaSmarwiStatus &status2)
{
	CPPUNIT_ASSERT_EQUAL(status1.status(), status2.status());
	CPPUNIT_ASSERT_EQUAL(status1.error(), status2.error());
	CPPUNIT_ASSERT_EQUAL(status1.ok(), status2.ok());
	CPPUNIT_ASSERT_EQUAL(status1.ro(), status2.ro());
	CPPUNIT_ASSERT_EQUAL(status1.pos(), status2.pos());
	CPPUNIT_ASSERT_EQUAL(status1.fix(), status2.fix());
	CPPUNIT_ASSERT_EQUAL(status1.ipAddress().toString(),
		status2.ipAddress().toString());
	CPPUNIT_ASSERT_EQUAL(status1.rssi(), status2.rssi());
}
}
