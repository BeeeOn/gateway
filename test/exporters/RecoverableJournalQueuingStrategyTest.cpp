#include <sstream>

#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Exception.h>

#include "cppunit/BetterAssert.h"
#include "cppunit/FileTestFixture.h"
#include "exporters/RecoverableJournalQueuingStrategy.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

static const string raw_b2d3703 =
	"5E64725C\t{\"device_id\":\"0x4100000001020304\",\"timestamp\":1527660187000000,\"data\":["
	"{\"module_id\":0,\"value\":5.000},{\"module_id\":1,\"value\":14.500},{\"module_id\":2,\"value\":-15.000}]}\n"
	"80EC1F84\t{\"device_id\":\"0x410000000a0b0c0d\",\"timestamp\":1527660231000000,\"data\":["
	"{\"module_id\":0,\"value\":1.000}]}\n"
	"C3E5786E\t{\"device_id\":\"0x4100000001020304\",\"timestamp\":1527661621000000,\"data\":["
	"{\"module_id\":0,\"value\":6.000},{\"module_id\":3,\"value\":1.000}]}\n";

static const string raw_6fef851 =
	"441BFD4E\t{\"device_id\":\"0x410000000fffffff\",\"timestamp\":1528012112000000,\"data\":["
	"{\"module_id\":0,\"value\":0.000},{\"module_id\":1,\"value\":1.000}]}\n"
	"6D22E082\t{\"device_id\":\"0x410000000a0b0c0d\",\"timestamp\":1528012123000000,\"data\":["
	"{\"module_id\":0,\"value\":0.000}]}\n";

static const string raw_3a8f509 =
	"5E64725C\t{\"device_id\":\"0x4100000001020304\",\"timestamp\":1527660187000000,\"data\":["
	"{\"module_id\":0,\"value\":5.000},"
	"{\"module_id\":1,\"value\":14.500},"
	"{\"module_id\":2,\"value\":-15.000}]}\n";

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

	File data0(Path(testingPath(), "b2d37030ae3d28d6fde6db21b43362ae54a35299"));
	writeFile(data0, raw_b2d3703);

	File data1(Path(testingPath(), "6fef851e64db0ceded0bb3043354855853c66f7d"));
	writeFile(data1, raw_6fef851);

	File index(Path(testingPath(), "index"));
	writeFile(index, "D29C989A\tb2d37030ae3d28d6fde6db21b43362ae54a35299\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT(!strategy.empty());

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"D29C989A\tb2d37030ae3d28d6fde6db21b43362ae54a35299\t0\n"
		"E3D31B2B\t6fef851e64db0ceded0bb3043354855853c66f7d\t0\n",
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

	File data0(Path(testingPath(), "b2d37030ae3d28d6fde6db21b43362ae54a35299"));
	writeFile(data0, raw_b2d3703);

	File tmpData(Path(testingPath(), "data.tmp"));
	writeFile(tmpData, raw_6fef851);

	File data1(Path(testingPath(), "6fef851e64db0ceded0bb3043354855853c66f7d"));

	File index(Path(testingPath(), "index"));
	writeFile(index, "D29C989A\tb2d37030ae3d28d6fde6db21b43362ae54a35299\t0\n");

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(data1);

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT(!strategy.empty());

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"D29C989A\tb2d37030ae3d28d6fde6db21b43362ae54a35299\t0\n"
		"E3D31B2B\t6fef851e64db0ceded0bb3043354855853c66f7d\t0\n",
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

	File data0(Path(testingPath(), "6fef851e64db0ceded0bb3043354855853c66f7d"));
	writeFile(data0, raw_6fef851);

	File tmpData(Path(testingPath(), "data.tmp"));
	writeFile(tmpData, raw_b2d3703.substr(0, 179));

	File data1(Path(testingPath(), "b2d37030ae3d28d6fde6db21b43362ae54a35299"));
	File data1Fixed(Path(testingPath(), "3a8f509275d7a56453fc8274e22789d6d15d8e78"));

	File index(Path(testingPath(), "index"));
	writeFile(index, "E3D31B2B\t6fef851e64db0ceded0bb3043354855853c66f7d\t0\n");

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(data1);

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT(!strategy.empty());

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"E3D31B2B\t6fef851e64db0ceded0bb3043354855853c66f7d\t0\n"
		"84445BBC\t3a8f509275d7a56453fc8274e22789d6d15d8e78\t0\n",
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

	File broken(Path(testingPath(), "b2d37030ae3d28d6fde6db21b43362ae54a35299"));
	writeFile(broken, raw_b2d3703.substr(0, 179));

	File index(Path(testingPath(), "index"));
	writeFile(index, "D29C989A\tb2d37030ae3d28d6fde6db21b43362ae54a35299\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());

	CPPUNIT_ASSERT_FILE_EXISTS(Path(testingPath(), "3a8f509275d7a56453fc8274e22789d6d15d8e78"));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
			raw_3a8f509,
			Path(testingPath(), "3a8f509275d7a56453fc8274e22789d6d15d8e78"));

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"D29C989A\tb2d37030ae3d28d6fde6db21b43362ae54a35299\t0\n"
		"84445BBC\t3a8f509275d7a56453fc8274e22789d6d15d8e78\t0\n"
		"EE9E1904\tb2d37030ae3d28d6fde6db21b43362ae54a35299\tdrop\n",
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

	File broken(Path(testingPath(), "b2d37030ae3d28d6fde6db21b43362ae54a35299"));
	writeFile(broken, raw_b2d3703.substr(0, 179));

	File recovered(Path(testingPath(), "3a8f509275d7a56453fc8274e22789d6d15d8e78"));
	writeFile(recovered, raw_3a8f509);

	File index(Path(testingPath(), "index"));
	writeFile(index,
		"D29C989A\tb2d37030ae3d28d6fde6db21b43362ae54a35299\t0\n"
		"84445BBC\t3a8f509275d7a56453fc8274e22789d6d15d8e78\t0\n");
		//"EE9E1904\tb2d37030ae3d28d6fde6db21b43362ae54a35299\tdrop\n" << missing this

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(Path(testingPath(), "b2d37030ae3d28d6fde6db21b43362ae54a35299"));

	CPPUNIT_ASSERT_FILE_EXISTS(Path(testingPath(), "3a8f509275d7a56453fc8274e22789d6d15d8e78"));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
			raw_3a8f509,
			Path(testingPath(), "3a8f509275d7a56453fc8274e22789d6d15d8e78"));

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"D29C989A\tb2d37030ae3d28d6fde6db21b43362ae54a35299\t0\n"
		"84445BBC\t3a8f509275d7a56453fc8274e22789d6d15d8e78\t0\n"
		"84445BBC\t3a8f509275d7a56453fc8274e22789d6d15d8e78\t0\n"
		"EE9E1904\tb2d37030ae3d28d6fde6db21b43362ae54a35299\tdrop\n",
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

	File broken(Path(testingPath(), "b2d37030ae3d28d6fde6db21b43362ae54a35299"));
	writeFile(broken, raw_b2d3703.substr(0, 179));

	File tmpData(Path(testingPath(), "data.tmp"));
	writeFile(tmpData, raw_6fef851);

	File index(Path(testingPath(), "index"));
	writeFile(index, "D29C989A\tb2d37030ae3d28d6fde6db21b43362ae54a35299\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());

	CPPUNIT_ASSERT_FILE_EXISTS(Path(testingPath(), "3a8f509275d7a56453fc8274e22789d6d15d8e78"));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
			raw_3a8f509,
			Path(testingPath(), "3a8f509275d7a56453fc8274e22789d6d15d8e78"));

	CPPUNIT_ASSERT_FILE_EXISTS(Path(testingPath(), "6fef851e64db0ceded0bb3043354855853c66f7d"));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
			raw_6fef851,
			Path(testingPath(), "6fef851e64db0ceded0bb3043354855853c66f7d"));

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(tmpData);

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"D29C989A\tb2d37030ae3d28d6fde6db21b43362ae54a35299\t0\n"
		"84445BBC\t3a8f509275d7a56453fc8274e22789d6d15d8e78\t0\n"
		"EE9E1904\tb2d37030ae3d28d6fde6db21b43362ae54a35299\tdrop\n"
		"E3D31B2B\t6fef851e64db0ceded0bb3043354855853c66f7d\t0\n",
		index);
}

}
