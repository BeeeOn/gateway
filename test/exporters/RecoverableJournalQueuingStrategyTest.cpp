#include <sstream>

#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Exception.h>

#include "cppunit/BetterAssert.h"
#include "cppunit/FileTestFixture.h"
#include "exporters/RecoverableJournalQueuingStrategy.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

static const string raw_7f23d5a =
	"D4EC89F4\t{\"device_id\":\"0x4100000001020304\",\"timestamp\":1527660187000000,\"data\":["
	"{\"module_id\":0,\"value\":5},{\"module_id\":1,\"value\":14.5},{\"module_id\":2,\"value\":-15}]}\n";

static const string raw_1e90a60 =
	"D4EC89F4\t{\"device_id\":\"0x4100000001020304\",\"timestamp\":1527660187000000,\"data\":["
	"{\"module_id\":0,\"value\":5},{\"module_id\":1,\"value\":14.5},{\"module_id\":2,\"value\":-15}]}\n"
	"3E4FD13E\t{\"device_id\":\"0x410000000a0b0c0d\",\"timestamp\":1527660231000000,\"data\":["
	"{\"module_id\":0,\"value\":1}]}\n"
	"178646E2\t{\"device_id\":\"0x4100000001020304\",\"timestamp\":1527661621000000,\"data\":["
	"{\"module_id\":0,\"value\":6},{\"module_id\":3,\"value\":1}]}\n";

static const string raw_fdd5085 =
	"46F3D928\t{\"device_id\":\"0x410000000fffffff\",\"timestamp\":1528012112000000,\"data\":["
	"{\"module_id\":0,\"value\":0},{\"module_id\":1,\"value\":1}]}\n"
	"1193DA2B\t{\"device_id\":\"0x410000000a0b0c0d\",\"timestamp\":1528012123000000,\"data\":["
	"{\"module_id\":0,\"value\":0}]}\n";

static const string raw_7f23d5f =
	"D4EC89F4\t{\"device_id\":\"0x4100000001020304\",\"timestamp\":1527660187000000,\"data\":["
	"{\"module_id\":0,\"value\":5},"
	"{\"module_id\":1,\"value\":14.5},"
	"{\"module_id\":2,\"value\":-15}]}\n";

class RecoverableJournalQueuingStrategyTest : public FileTestFixture {
	CPPUNIT_TEST_SUITE(RecoverableJournalQueuingStrategyTest);
	CPPUNIT_TEST(testRecoverBufferNotInIndex);
	CPPUNIT_TEST(testRecoverTmpData);
	CPPUNIT_TEST(testRecoverIncompleteTmpData);
	CPPUNIT_TEST(testRecoverPartially);
	CPPUNIT_TEST(testRecoverInterruptedRecover);
	CPPUNIT_TEST(testRecoverWhileHavingTmpData);
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
	void testRecoverBufferNotInIndex();
	void testRecoverTmpData();
	void testRecoverIncompleteTmpData();
	void testRecoverPartially();
	void testRecoverInterruptedRecover();
	void testRecoverWhileHavingTmpData();
};

CPPUNIT_TEST_SUITE_REGISTRATION(RecoverableJournalQueuingStrategyTest);

void RecoverableJournalQueuingStrategyTest::setUp()
{
	FileTestFixture::setUpAsDirectory();
}

/**
 * @brief Test recovery of the most recent data. If a buffer is commited but not
 * written to index (e.g. power supply failure), it would be lost. The recovery
 * process should find such buffer and append it to index.
 */
void RecoverableJournalQueuingStrategyTest::testRecoverBufferNotInIndex()
{
	RecoverableJournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());
	strategy.setDisableGC(true);

	File data0(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	writeFile(data0, raw_1e90a60);

	File data1(Path(testingPath(), "fdd5085abed67887ce412239738352fc3ae3936f"));
	writeFile(data1, raw_fdd5085);

	File index(Path(testingPath(), "index"));
	writeFile(index, "60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT(!strategy.empty());

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n",
		index);
}

/**
 * @brief Test recovery when data.tmp file is present. This simulates unexpected
 * power supply failure as the data.tmp file represents an uncommitted buffer.
 */
void RecoverableJournalQueuingStrategyTest::testRecoverTmpData()
{
	RecoverableJournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());
	strategy.setDisableGC(true);

	File data0(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	writeFile(data0, raw_1e90a60);

	File tmpData(Path(testingPath(), "data.tmp"));
	writeFile(tmpData, raw_fdd5085);

	File data1(Path(testingPath(), "fdd5085abed67887ce412239738352fc3ae3936f"));

	File index(Path(testingPath(), "index"));
	writeFile(index, "60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n");

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(data1);

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT(!strategy.empty());

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n",
		index);

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(tmpData);
	CPPUNIT_ASSERT_FILE_EXISTS(data1);
}

/**
 * @brief Test recovery when data.tmp file is present. This simulates unexpected
 * power supply failure as the data.tmp file is incomplete and uncommitted.
 * We should partially recover it and append it to index properly.
 */
void RecoverableJournalQueuingStrategyTest::testRecoverIncompleteTmpData()
{
	RecoverableJournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());
	strategy.setDisableGC(true);

	File data0(Path(testingPath(), "fdd5085abed67887ce412239738352fc3ae3936f"));
	writeFile(data0, raw_fdd5085);

	File tmpData(Path(testingPath(), "data.tmp"));
	writeFile(tmpData, raw_1e90a60.substr(0, 168));

	File data1(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	File data1Fixed(Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));

	File index(Path(testingPath(), "index"));
	writeFile(index, "62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n");

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(data1);

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT(!strategy.empty());

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n"
		"24D75BA2\t7f23d5f8aa61ea540c4af41b59381a054dc0601d\t0\n",
		index);

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(tmpData);
	CPPUNIT_ASSERT_FILE_NOT_EXISTS(data1);
	CPPUNIT_ASSERT_FILE_EXISTS(data1Fixed);
}

/**
 * @brief Test recovery of a broken buffer referenced from index. We should recover
 * the broken buffer and then drop the previous one.
 */
void RecoverableJournalQueuingStrategyTest::testRecoverPartially()
{
	RecoverableJournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());
	strategy.setDisableGC(true);

	File broken(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	writeFile(broken, raw_1e90a60.substr(0, 168));

	File index(Path(testingPath(), "index"));
	writeFile(index, "60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());

	CPPUNIT_ASSERT_FILE_EXISTS(Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
			raw_7f23d5a,
			Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"24D75BA2\t7f23d5f8aa61ea540c4af41b59381a054dc0601d\t0\n"
		"521E6294\t1e90a6059b538bb614b762d1f94203fafb3533d6\tdrop\n",
		index);
}

/**
 * @brief Test what happens when a recovery is interrupted while recovering a buffer
 * partially. The buffer is recovered but not written into the index. Thus, we would
 * do the recovery again and duplicate the record in index.
 */
void RecoverableJournalQueuingStrategyTest::testRecoverInterruptedRecover()
{
	RecoverableJournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());
	strategy.setDisableGC(true);

	File broken(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	writeFile(broken, raw_1e90a60.substr(0, 168));

	File recovered(Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));
	writeFile(recovered, raw_7f23d5f);

	File index(Path(testingPath(), "index"));
	writeFile(index,
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"24D75BA2\t7f23d5f8aa61ea540c4af41b59381a054dc0601d\t0\n");
		//"521E6294\t1e90a6059b538bb614b762d1f94203fafb3533d6\tdrop\n" << missing this

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));

	CPPUNIT_ASSERT_FILE_EXISTS(Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
			raw_7f23d5a,
			Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"24D75BA2\t7f23d5f8aa61ea540c4af41b59381a054dc0601d\t0\n"
		"24D75BA2\t7f23d5f8aa61ea540c4af41b59381a054dc0601d\t0\n"
		"521E6294\t1e90a6059b538bb614b762d1f94203fafb3533d6\tdrop\n",
		index);
}

/**
 * @brief Test recovery process when there is a broken buffer referenced from index and
 * the data.tmp file exists. The data.tmp must not be rewritten by the recovery process.
 */
void RecoverableJournalQueuingStrategyTest::testRecoverWhileHavingTmpData()
{
	RecoverableJournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());
	strategy.setDisableGC(true);

	File broken(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	writeFile(broken, raw_1e90a60.substr(0, 168));

	File tmpData(Path(testingPath(), "data.tmp"));
	writeFile(tmpData, raw_fdd5085);

	File index(Path(testingPath(), "index"));
	writeFile(index, "60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());

	CPPUNIT_ASSERT_FILE_EXISTS(Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
			raw_7f23d5a,
			Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));

	CPPUNIT_ASSERT_FILE_EXISTS(Path(testingPath(), "fdd5085abed67887ce412239738352fc3ae3936f"));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
			raw_fdd5085,
			Path(testingPath(), "fdd5085abed67887ce412239738352fc3ae3936f"));

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(tmpData);

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"24D75BA2\t7f23d5f8aa61ea540c4af41b59381a054dc0601d\t0\n"
		"521E6294\t1e90a6059b538bb614b762d1f94203fafb3533d6\tdrop\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n",
		index);
}

}
