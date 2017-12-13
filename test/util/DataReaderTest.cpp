#include <cppunit/extensions/HelperMacros.h>
#include "cppunit/BetterAssert.h"

#include "util/DataReader.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class DataReaderTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DataReaderTest);
	CPPUNIT_TEST(testRead);
	CPPUNIT_TEST(testSkip);
	CPPUNIT_TEST(testEmptyInput);
	CPPUNIT_TEST(testInvalidChecksum);
	CPPUNIT_TEST_SUITE_END();
public:
	void testRead();
	void testSkip();
	void testEmptyInput();
	void testInvalidChecksum();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DataReaderTest);

void DataReaderTest::testRead()
{
	stringstream testStream;

	testStream << "646AB873first string" << endl
	           << "9851078CSECOND STRING" << endl;

	DataReader reader(testStream);

	CPPUNIT_ASSERT(reader.hasNext());
	CPPUNIT_ASSERT_EQUAL("first string", reader.next());
	CPPUNIT_ASSERT_EQUAL(1, reader.dataRead());

	CPPUNIT_ASSERT(reader.hasNext());
	CPPUNIT_ASSERT_EQUAL("SECOND STRING", reader.next());
	CPPUNIT_ASSERT_EQUAL(2, reader.dataRead());

	CPPUNIT_ASSERT(!reader.hasNext());
	CPPUNIT_ASSERT_THROW(reader.next(), IllegalStateException);
	CPPUNIT_ASSERT_EQUAL(2, reader.dataRead());
}

void DataReaderTest::testSkip()
{
	stringstream testStream;

	testStream << "646AB873first string" << endl
	           << "9851078CSECOND STRING" << endl;

	DataReader reader(testStream);

	CPPUNIT_ASSERT_EQUAL(1, reader.skip(1));

	CPPUNIT_ASSERT(reader.hasNext());
	CPPUNIT_ASSERT_EQUAL("SECOND STRING", reader.next());
	CPPUNIT_ASSERT_EQUAL(1, reader.dataRead());

	CPPUNIT_ASSERT(!reader.hasNext());
	CPPUNIT_ASSERT_THROW(reader.next(), IllegalStateException);
	CPPUNIT_ASSERT_EQUAL(1, reader.dataRead());
}

void DataReaderTest::testEmptyInput()
{
	stringstream testStream;

	DataReader reader(testStream);

	CPPUNIT_ASSERT(!reader.hasNext());
	CPPUNIT_ASSERT_THROW(reader.next(), IllegalStateException);
	CPPUNIT_ASSERT_EQUAL(0, reader.dataRead());
}

void DataReaderTest::testInvalidChecksum()
{
	stringstream testStream;

	testStream << "00000000first string" << endl
	           << "9851078CSECOND STRING" << endl;

	DataReader reader(testStream);

	CPPUNIT_ASSERT(reader.hasNext());
	CPPUNIT_ASSERT_EQUAL("SECOND STRING", reader.next());
	CPPUNIT_ASSERT_EQUAL(1, reader.dataRead());

	CPPUNIT_ASSERT(!reader.hasNext());
	CPPUNIT_ASSERT_THROW(reader.next(), IllegalStateException);
	CPPUNIT_ASSERT_EQUAL(1, reader.dataRead());
}

}
