#include <sstream>

#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Path.h>

#include "cppunit/BetterAssert.h"
#include "cppunit/FileTestFixture.h"
#include "util/Journal.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class JournalTest : public FileTestFixture {
	CPPUNIT_TEST_SUITE(JournalTest);
	CPPUNIT_TEST(testLoadEmpty);
	CPPUNIT_TEST(testLoadInvalid);
	CPPUNIT_TEST(testLoadInterpret);
	CPPUNIT_TEST(testLoadRecover);
	CPPUNIT_TEST(testLoad);
	CPPUNIT_TEST(testAppend);
	CPPUNIT_TEST(testAppendBatch);
	CPPUNIT_TEST(testDrop);
	CPPUNIT_TEST(testDropBatch);
	CPPUNIT_TEST(testDuplicatesFactor);
	CPPUNIT_TEST(testAppendWithRewrite);
	CPPUNIT_TEST(testEatMyself);
	CPPUNIT_TEST(testCheckConsistent);
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
	void testLoadEmpty();
	void testLoadInvalid();
	void testLoadInterpret();
	void testLoadRecover();
	void testLoad();
	void testAppend();
	void testAppendBatch();
	void testDrop();
	void testDropBatch();
	void testDuplicatesFactor();
	void testAppendWithRewrite();
	void testEatMyself();
	void testCheckConsistent();
};

CPPUNIT_TEST_SUITE_REGISTRATION(JournalTest);

void JournalTest::setUp()
{
	FileTestFixture::setUp();
	writeFile(testingFile(),
		"414FF3E0\ta\t0\n"
		"551C80BB\ta\t256\n"
		"43094DB9\tb\t0\n"
		"63E36418\tc\tdrop\n"
		"4784310B\td\t0\n"
		"86A8AB1B\tb\t200\n"
		"BAD08BA0\ta\t354\n"
		"42CB278E\tc\t0\n"
		"A8BFB7BD\tb\tdrop\n"
		"F75AD3E8\td\t56\n");
}

void JournalTest::testLoadEmpty()
{
	istringstream empty;
	Journal journal(testingPath());

	CPPUNIT_ASSERT(journal.records().empty());

	journal.load(empty);

	CPPUNIT_ASSERT(journal.records().empty());
}

void JournalTest::testLoadInvalid()
{
	istringstream invalid;
	Journal journal(testingPath());

	CPPUNIT_ASSERT(journal.records().empty());

	invalid.str("pjoeihgoegheoigjepgoepr");

	CPPUNIT_ASSERT_THROW(journal.load(invalid), InvalidArgumentException);
	CPPUNIT_ASSERT(journal.records().empty());

	invalid.clear();
	invalid.str("00000000\tsomeid\t0\n");

	CPPUNIT_ASSERT_THROW(journal.load(invalid), IllegalStateException);
	CPPUNIT_ASSERT(journal.records().empty());

	invalid.clear();
	invalid.str(
		"2150C13F\tsomeid\t0\n"
		"broken stuff\n");

	CPPUNIT_ASSERT_THROW(journal.load(invalid), InvalidArgumentException);
	CPPUNIT_ASSERT(journal.records().empty());
}

void JournalTest::testLoadInterpret()
{
	istringstream input;
	Journal journal(testingPath());

	CPPUNIT_ASSERT(journal.records().empty());

	input.str(
		"414FF3E0\ta\t0\n"
		"551C80BB\ta\t256\n"
		"43094DB9\tb\t0\n"
		"63E36418\tc\tdrop\n"
		"4784310B\td\t0\n"
		"86A8AB1B\tb\t200\n"
		"BAD08BA0\ta\t354\n"
		"42CB278E\tc\t0\n"
		"A8BFB7BD\tb\tdrop\n"
		"F75AD3E8\td\t56\n");

	journal.load(input);

	const auto &records = journal.records();
	CPPUNIT_ASSERT_EQUAL(3, records.size());

	CPPUNIT_ASSERT_EQUAL("354", journal["a"].value());
	CPPUNIT_ASSERT(journal["b"].isNull());
	CPPUNIT_ASSERT_EQUAL("0", journal["c"].value());
	CPPUNIT_ASSERT_EQUAL("56", journal["d"].value());

	auto it = records.begin();
	CPPUNIT_ASSERT(it != records.end());
	CPPUNIT_ASSERT_EQUAL("a", it->key);
	CPPUNIT_ASSERT_EQUAL("354", it->value);

	CPPUNIT_ASSERT(++it != records.end());
	CPPUNIT_ASSERT_EQUAL("d", it->key);
	CPPUNIT_ASSERT_EQUAL("56", it->value);

	CPPUNIT_ASSERT(++it != records.end());
	CPPUNIT_ASSERT_EQUAL("c", it->key);
	CPPUNIT_ASSERT_EQUAL("0", it->value);

	CPPUNIT_ASSERT(++it == records.end());
}

void JournalTest::testLoadRecover()
{
	istringstream input;
	Journal journal(testingPath());

	CPPUNIT_ASSERT(journal.records().empty());

	input.str(
		"414FF3E0\ta\t0\n"
		"551C80BB\ta\t256\n"
		"broken line\n"
		"63E36418\tc\tdrop\n"
		"4784310B\td\t0\n"
		"86A8AB1B\tb\t200\n"
		"BAD08BA0\ta\t354\n"
		"broken line\n"
		"A8BFB7BD\tb\tdrop\n"
		"broken line\n");

	journal.load(input, true);

	const auto &records = journal.records();
	CPPUNIT_ASSERT_EQUAL(2, records.size());

	CPPUNIT_ASSERT_EQUAL("354", journal["a"].value());
	CPPUNIT_ASSERT(journal["b"].isNull());
	CPPUNIT_ASSERT(journal["c"].isNull());
	CPPUNIT_ASSERT_EQUAL("0", journal["d"].value());

	auto it = records.begin();
	CPPUNIT_ASSERT(it != records.end());
	CPPUNIT_ASSERT_EQUAL("a", it->key);
	CPPUNIT_ASSERT_EQUAL("354", it->value);

	CPPUNIT_ASSERT(++it != records.end());
	CPPUNIT_ASSERT_EQUAL("d", it->key);
	CPPUNIT_ASSERT_EQUAL("0", it->value);

	CPPUNIT_ASSERT(++it == records.end());
}

void JournalTest::testLoad()
{
	Journal journal(testingPath());
	journal.load();

	CPPUNIT_ASSERT_EQUAL("354", journal["a"].value());
	CPPUNIT_ASSERT(journal["b"].isNull());
	CPPUNIT_ASSERT_EQUAL("0", journal["c"].value());
	CPPUNIT_ASSERT_EQUAL("56", journal["d"].value());

	const auto &records = journal.records();
	auto it = records.begin();
	CPPUNIT_ASSERT(it != records.end());
	CPPUNIT_ASSERT_EQUAL("a", it->key);
	CPPUNIT_ASSERT_EQUAL("354", it->value);

	CPPUNIT_ASSERT(++it != records.end());
	CPPUNIT_ASSERT_EQUAL("d", it->key);
	CPPUNIT_ASSERT_EQUAL("56", it->value);

	CPPUNIT_ASSERT(++it != records.end());
	CPPUNIT_ASSERT_EQUAL("c", it->key);
	CPPUNIT_ASSERT_EQUAL("0", it->value);

	CPPUNIT_ASSERT(++it == records.end());
}

void JournalTest::testAppend()
{
	Journal journal(testingPath());
	journal.load();

	journal.append("a", "671");

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"414FF3E0\ta\t0\n"
		"551C80BB\ta\t256\n"
		"43094DB9\tb\t0\n"
		"63E36418\tc\tdrop\n"
		"4784310B\td\t0\n"
		"86A8AB1B\tb\t200\n"
		"BAD08BA0\ta\t354\n"
		"42CB278E\tc\t0\n"
		"A8BFB7BD\tb\tdrop\n"
		"F75AD3E8\td\t56\n"
		"FE47DF46\ta\t671\n",
		testingPath());
}

void JournalTest::testAppendBatch()
{
	Journal journal(testingPath());
	journal.load();

	journal.append("a", "671", false);
	journal.append("c", "11", false);
	journal.append("a", "1000", false);

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"414FF3E0\ta\t0\n"
		"551C80BB\ta\t256\n"
		"43094DB9\tb\t0\n"
		"63E36418\tc\tdrop\n"
		"4784310B\td\t0\n"
		"86A8AB1B\tb\t200\n"
		"BAD08BA0\ta\t354\n"
		"42CB278E\tc\t0\n"
		"A8BFB7BD\tb\tdrop\n"
		"F75AD3E8\td\t56\n",
		testingPath());

	journal.flush();

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"414FF3E0\ta\t0\n"
		"551C80BB\ta\t256\n"
		"43094DB9\tb\t0\n"
		"63E36418\tc\tdrop\n"
		"4784310B\td\t0\n"
		"86A8AB1B\tb\t200\n"
		"BAD08BA0\ta\t354\n"
		"42CB278E\tc\t0\n"
		"A8BFB7BD\tb\tdrop\n"
		"F75AD3E8\td\t56\n"
		"FE47DF46\ta\t671\n"
		"9085BBF6\tc\t11\n"
		"EA196326\ta\t1000\n",
		testingPath());
}

void JournalTest::testDrop()
{
	Journal journal(testingPath());
	journal.load();

	journal.drop("a");

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"414FF3E0\ta\t0\n"
		"551C80BB\ta\t256\n"
		"43094DB9\tb\t0\n"
		"63E36418\tc\tdrop\n"
		"4784310B\td\t0\n"
		"86A8AB1B\tb\t200\n"
		"BAD08BA0\ta\t354\n"
		"42CB278E\tc\t0\n"
		"A8BFB7BD\tb\tdrop\n"
		"F75AD3E8\td\t56\n"
		"2E2BC513\ta\tdrop\n",
		testingPath());
}

void JournalTest::testDropBatch()
{
	Journal journal(testingPath());
	journal.load();

	journal.drop("a", false);
	journal.drop("c", false);

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"414FF3E0\ta\t0\n"
		"551C80BB\ta\t256\n"
		"43094DB9\tb\t0\n"
		"63E36418\tc\tdrop\n"
		"4784310B\td\t0\n"
		"86A8AB1B\tb\t200\n"
		"BAD08BA0\ta\t354\n"
		"42CB278E\tc\t0\n"
		"A8BFB7BD\tb\tdrop\n"
		"F75AD3E8\td\t56\n",
		testingPath());

	journal.flush();

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"414FF3E0\ta\t0\n"
		"551C80BB\ta\t256\n"
		"43094DB9\tb\t0\n"
		"63E36418\tc\tdrop\n"
		"4784310B\td\t0\n"
		"86A8AB1B\tb\t200\n"
		"BAD08BA0\ta\t354\n"
		"42CB278E\tc\t0\n"
		"A8BFB7BD\tb\tdrop\n"
		"F75AD3E8\td\t56\n"
		"2E2BC513\ta\tdrop\n"
		"63E36418\tc\tdrop\n",
		testingPath());
}

void JournalTest::testDuplicatesFactor()
{
	istringstream input;
	Journal journal(testingPath());

	CPPUNIT_ASSERT(journal.records().empty());

	CPPUNIT_ASSERT_EQUAL(1.0, journal.currentDuplicatesFactor());

	journal.append("a", "0");
	CPPUNIT_ASSERT_EQUAL(1.0, journal.currentDuplicatesFactor());

	journal.append("a", "150");
	CPPUNIT_ASSERT_EQUAL(2.0, journal.currentDuplicatesFactor());

	journal.append("b", "0");
	CPPUNIT_ASSERT_EQUAL(1.5, journal.currentDuplicatesFactor());

	journal.append("b", "14");
	CPPUNIT_ASSERT_EQUAL(2.0, journal.currentDuplicatesFactor());

	journal.append("c", "0");
	journal.append("c", "163");
	CPPUNIT_ASSERT_EQUAL(2.0, journal.currentDuplicatesFactor());
}

void JournalTest::testAppendWithRewrite()
{
	Journal journal(testingPath(), 1.0, 32);
	journal.load();

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"414FF3E0\ta\t0\n"
		"551C80BB\ta\t256\n"
		"43094DB9\tb\t0\n"
		"63E36418\tc\tdrop\n"
		"4784310B\td\t0\n"
		"86A8AB1B\tb\t200\n"
		"BAD08BA0\ta\t354\n"
		"42CB278E\tc\t0\n"
		"A8BFB7BD\tb\tdrop\n"
		"F75AD3E8\td\t56\n",
		testingPath());

	journal.append("a", "671");
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"FE47DF46\ta\t671\n"
		"F75AD3E8\td\t56\n"
		"42CB278E\tc\t0\n",
		testingPath());

	// no rewrite occurs because the main records have no duplicates
	journal.append("a", "1671");
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"FE47DF46\ta\t671\n"
		"F75AD3E8\td\t56\n"
		"42CB278E\tc\t0\n"
		"D6D2B9C5\ta\t1671\n",
		testingPath());

	journal.append("d", "10127");
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"D6D2B9C5\ta\t1671\n"
		"D994F10E\td\t10127\n"
		"42CB278E\tc\t0\n",
		testingPath());

	// no rewrite occurs because the main records have no duplicates
	journal.append("a", "512");
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"D6D2B9C5\ta\t1671\n"
		"D994F10E\td\t10127\n"
		"42CB278E\tc\t0\n"
		"33529723\ta\t512\n",
		testingPath());

	journal.drop("d");
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"33529723\ta\t512\n"
		"42CB278E\tc\t0\n",
		testingPath());
}

void JournalTest::testEatMyself()
{
	Journal journal(testingPath());

	writeFile(testingFile(), "");
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS("", testingFile());

	journal.append("a", "0");
	journal.append("b", "0");
	journal.append("a", "156");
	journal.append("d", "0");
	journal.drop("a");
	journal.append("c", "0");
	journal.append("d", "789");
	journal.append("d", "1023");
	journal.append("c", "119");

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"414FF3E0\ta\t0\n"
		"43094DB9\tb\t0\n"
		"575A3EE2\ta\t156\n"
		"4784310B\td\t0\n"
		"2E2BC513\ta\tdrop\n"
		"42CB278E\tc\t0\n"
		"BE26AEFC\td\t789\n"
		"11EBC1AD\td\t1023\n"
		"D949B517\tc\t119\n",
		testingPath());

	CPPUNIT_ASSERT_NO_THROW(journal.load());

	const auto &records = journal.records();
	auto it = records.begin();
	CPPUNIT_ASSERT(it != records.end());
	CPPUNIT_ASSERT_EQUAL("b", it->key);
	CPPUNIT_ASSERT_EQUAL("0", it->value);

	CPPUNIT_ASSERT(++it != records.end());
	CPPUNIT_ASSERT_EQUAL("d", it->key);
	CPPUNIT_ASSERT_EQUAL("1023", it->value);

	CPPUNIT_ASSERT(++it != records.end());
	CPPUNIT_ASSERT_EQUAL("c", it->key);
	CPPUNIT_ASSERT_EQUAL("119", it->value);

	CPPUNIT_ASSERT(++it == records.end());
}

void JournalTest::testCheckConsistent()
{
	istringstream input;
	Journal journal(testingPath());
	journal.load();

	CPPUNIT_ASSERT_EQUAL(3, journal.records().size());

	input.clear();
	input.str("");
	CPPUNIT_ASSERT_THROW(journal.checkConsistent(input), IllegalStateException);

	input.clear();
	input.str(
		"414FF3E0\ta\t0\n"
		"551C80BB\ta\t256\n"
		"43094DB9\tb\t0\n");
	CPPUNIT_ASSERT_THROW(journal.checkConsistent(input), IllegalStateException);

	input.clear();
	input.str(
		"414FF3E0\ta\t0\n"
		"551C80BB\ta\t256\n"
		"43094DB9\tb\t0\n"
		"63E36418\tc\tdrop\n"
		"4784310B\td\t0\n"
		"86A8AB1B\tb\t200\n"
		"BAD08BA0\ta\t354\n"
		"42CB278E\tc\t0\n"
		"A8BFB7BD\tb\tdrop\n"
		"F75AD3E8\td\t56\n");

	CPPUNIT_ASSERT_NO_THROW(journal.checkConsistent(input));

	input.clear();
	input.str(
		"414FF3E0\ta\t0\n"
		"551C80BB\ta\t256\n"
		"43094DB9\tb\t0\n"
		"00000000\tx\tinvalid\n" // invalid record is skipped
		"63E36418\tc\tdrop\n"
		"4784310B\td\t0\n"
		"86A8AB1B\tb\t200\n"
		"BAD08BA0\ta\t354\n"
		"42CB278E\tc\t0\n"
		"A8BFB7BD\tb\tdrop\n"
		"F75AD3E8\td\t56\n");

	CPPUNIT_ASSERT_NO_THROW(journal.checkConsistent(input));
}

}
