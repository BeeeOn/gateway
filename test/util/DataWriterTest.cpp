#include <cppunit/extensions/HelperMacros.h>
#include "cppunit/BetterAssert.h"

#include "util/DataWriter.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class DataWriterTestIterator : public DataIterator {
public:
	DataWriterTestIterator(deque<string> &data):
			m_data(data)
	{
	}

	bool hasNext() override
	{
		return !m_data.empty();
	}

	string next() override
	{
		string data = m_data.front();
		m_data.pop_front();
		return data;
	}

private:
	deque<string> m_data;
};

class DataWriterTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(DataWriterTest);
	CPPUNIT_TEST(testWrite);
	CPPUNIT_TEST(testWriteEmpty);
	CPPUNIT_TEST_SUITE_END();
public:
	void testWrite();
	void testWriteEmpty();
};

CPPUNIT_TEST_SUITE_REGISTRATION(DataWriterTest);

void DataWriterTest::testWrite()
{
	stringstream testStream;
	string string1("first string");
	string string2("SECOND STRING");

	DataWriter writer(testStream);

	deque<string> deque;

	deque.emplace_back(string1);
	deque.emplace_back(string2);

	DataWriterTestIterator itr(deque);

	writer.write(itr);

	CPPUNIT_ASSERT_EQUAL("646AB873first string\n"
			             "9851078CSECOND STRING\n",
	                     testStream.str());
}

void DataWriterTest::testWriteEmpty()
{
	stringstream testStream;

	DataWriter writer(testStream);

	deque<string> deque;
	CPPUNIT_ASSERT(deque.empty());

	DataWriterTestIterator itr(deque);

	writer.write(itr);

	CPPUNIT_ASSERT(testStream.str().empty());
}

}
