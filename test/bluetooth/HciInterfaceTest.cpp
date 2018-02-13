#include <cppunit/extensions/HelperMacros.h>

#include "bluetooth/HciInterface.h"

#define EIR_NAME_SHORT 0x08
#define EIR_NAME_COMPLETE 0x09

using namespace Poco;
using namespace std;

namespace BeeeOn {

class HciInterfaceTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(HciInterfaceTest);
	CPPUNIT_TEST(testParseLENameComplete);
	CPPUNIT_TEST(testParseLENameShort);
	CPPUNIT_TEST(testParseLENameEmpty);
	CPPUNIT_TEST(testParseLENameWrongLength);
	CPPUNIT_TEST_SUITE_END();
public:
	void testParseLENameComplete();
	void testParseLENameShort();
	void testParseLENameEmpty();
	void testParseLENameWrongLength();
};

CPPUNIT_TEST_SUITE_REGISTRATION(HciInterfaceTest);

class TestableHciInterface : public HciInterface {
public:
	TestableHciInterface(const std::string &name):
		HciInterface(name)
	{
	}
	using HciInterface::parseLEName;
};

void HciInterfaceTest::testParseLENameComplete()
{
	string ifname = "hci0";
	size_t length = 6;
	uint8_t eir[6];
	eir[0] = 5;
	eir[1] = EIR_NAME_COMPLETE;
	eir[2] = 'I';
	eir[3] = 'T';
	eir[4] = 'A';
	eir[5] = 'G';

	string str = TestableHciInterface::parseLEName(eir, length);

	CPPUNIT_ASSERT_EQUAL(string("ITAG"), str);
}

void HciInterfaceTest::testParseLENameShort()
{
	string ifname = "hci0";
	size_t length = 6;
	uint8_t eir[6];
	eir[0] = 5;
	eir[1] = EIR_NAME_SHORT;
	eir[2] = 'I';
	eir[3] = 'T';
	eir[4] = 'A';
	eir[5] = 'G';

	string str = TestableHciInterface::parseLEName(eir, length);

	CPPUNIT_ASSERT_EQUAL(string("ITAG"), str);
}

void HciInterfaceTest::testParseLENameEmpty()
{
	string ifname = "hci0";
	size_t length = 14;
	uint8_t eir[14];
	// non-name field of length 2
	eir[0] = 2;
	eir[1] = 1;
	eir[2] = 6;
	// non-name field of length 5
	eir[3] = 5;
	eir[4] = 3;
	eir[5] = 101;
	eir[6] = 254;
	eir[7] = 51;
	eir[8] = 254;
	// non-name field of length 4
	eir[9] = 4;
	eir[10] = 22;
	eir[11] = 101;
	eir[12] = 254;
	eir[13] = 3;

	string str = TestableHciInterface::parseLEName(eir, length);

	CPPUNIT_ASSERT_EQUAL(string(""), str);
}

void HciInterfaceTest::testParseLENameWrongLength()
{
	string ifname = "hci0";
	size_t length = 4;
	uint8_t eir[6];
	eir[0] = 5;
	eir[1] = EIR_NAME_COMPLETE;
	eir[2] = 'I';
	eir[3] = 'T';
	eir[4] = 'A';
	eir[5] = 'G';

	string str = TestableHciInterface::parseLEName(eir, length);

	CPPUNIT_ASSERT_EQUAL(string(""), str);
}

}
