#include <sstream>

#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Error.h>
#include <Poco/Exception.h>

#include "cppunit/BetterAssert.h"
#include "cppunit/FileTestFixture.h"
#include "exporters/JournalQueuingStrategy.h"
#include "io/SafeWriter.h"
#include "util/ChecksumSensorDataFormatter.h"
#include "util/ChecksumSensorDataParser.h"
#include "util/JSONSensorDataFormatter.h"
#include "util/JSONSensorDataParser.h"
#include "util/PosixSignal.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

static const vector<SensorData> data_1e90a60 = {
	{
		DeviceID::parse("0x4100000001020304"),
		Timestamp::fromEpochTime(1527660187),
		{{0, 5}, {1, 14.5}, {2, -15}}
	},
	{
		DeviceID::parse("0x410000000a0b0c0d"),
		Timestamp::fromEpochTime(1527660231),
		{{0, 1}}
	},
	{
		DeviceID::parse("0x4100000001020304"),
		Timestamp::fromEpochTime(1527661621),
		{{0, 6}, {3, 1}}
	},
};

static const string raw_1e90a60 =
	"D4EC89F4\t{\"device_id\":\"0x4100000001020304\",\"timestamp\":1527660187000000,\"data\":["
	"{\"module_id\":0,\"value\":5},{\"module_id\":1,\"value\":14.5},{\"module_id\":2,\"value\":-15}]}\n"
	"3E4FD13E\t{\"device_id\":\"0x410000000a0b0c0d\",\"timestamp\":1527660231000000,\"data\":["
	"{\"module_id\":0,\"value\":1}]}\n"
	"178646E2\t{\"device_id\":\"0x4100000001020304\",\"timestamp\":1527661621000000,\"data\":["
	"{\"module_id\":0,\"value\":6},{\"module_id\":3,\"value\":1}]}\n";

static const vector<SensorData> data_fdd5085 = {
	{
		DeviceID::parse("0x410000000fffffff"),
		Timestamp::fromEpochTime(1528012112),
		{{0, 0}, {1, 1}}
	},
	{
		DeviceID::parse("0x410000000a0b0c0d"),
		Timestamp::fromEpochTime(1528012123),
		{{0, 0}}
	}
};

static const string raw_fdd5085 =
	"46F3D928\t{\"device_id\":\"0x410000000fffffff\",\"timestamp\":1528012112000000,\"data\":["
	"{\"module_id\":0,\"value\":0},{\"module_id\":1,\"value\":1}]}\n"
	"1193DA2B\t{\"device_id\":\"0x410000000a0b0c0d\",\"timestamp\":1528012123000000,\"data\":["
	"{\"module_id\":0,\"value\":0}]}\n";

static const vector<SensorData> data_7f23d5f = {
	{
		DeviceID::parse("0x4100000001020304"),
		Timestamp::fromEpochTime(1527660187),
		{{0, 5}, {1, 14.5}, {2, -15}}
	}
};

static const string raw_7f23d5f =
	"D4EC89F4\t{\"device_id\":\"0x4100000001020304\",\"timestamp\":1527660187000000,\"data\":["
	"{\"module_id\":0,\"value\":5},"
	"{\"module_id\":1,\"value\":14.5},"
	"{\"module_id\":2,\"value\":-15}]}\n";

static const vector<SensorData> data_263eb6d = {
	{
		DeviceID::parse("0x410000000aaaaaaa"),
		Timestamp::fromEpochTime(1527695312),
		{{0, 1}, {1, 1}, {2, 1}}
	}
};

class JournalQueuingStrategyTest : public FileTestFixture {
	CPPUNIT_TEST_SUITE(JournalQueuingStrategyTest);
	CPPUNIT_TEST(testTestingData);
	CPPUNIT_TEST(testSetupFromScratch);
	CPPUNIT_TEST(testSetupExistingEmpty);
	CPPUNIT_TEST(testSetupExisting);
	CPPUNIT_TEST(testSetupWithBroken);
	CPPUNIT_TEST(testPushSuccessful);
	CPPUNIT_TEST(testPushNotWritable);
	CPPUNIT_TEST(testPushDiskFullOnIndexAppend);
	CPPUNIT_TEST(testPushLockExists);
	CPPUNIT_TEST(testPushOverSizeWithGC);
	CPPUNIT_TEST(testPushOverSizeNoGC);
	CPPUNIT_TEST(testPushOverRLimit);
	CPPUNIT_TEST(testRepeatedPeekStable);
	CPPUNIT_TEST(testPopFromEmpty);
	CPPUNIT_TEST(testPopZero);
	CPPUNIT_TEST(testPopAtOnce);
	CPPUNIT_TEST(testPopInSteps);
	CPPUNIT_TEST_SUITE_END();
public:
	void setUp();
	void testTestingData();
	void testSetupFromScratch();
	void testSetupExistingEmpty();
	void testSetupExisting();
	void testSetupWithBroken();
	void testPushSuccessful();
	void testPushNotWritable();
	void testPushDiskFullOnIndexAppend();
	void testPushLockExists();
	void testPushOverSizeWithGC();
	void testPushOverSizeNoGC();
	void testPushOverRLimit();
	void testRepeatedPeekStable();
	void testPopFromEmpty();
	void testPopZero();
	void testPopAtOnce();
	void testPopInSteps();
private:
	void doTestData(
		const vector<SensorData> &data,
		const string &raw,
		const string &hash) const;
};

CPPUNIT_TEST_SUITE_REGISTRATION(JournalQueuingStrategyTest);

/**
 * @brief Prepare the temporary file as a directory because the JournalQueuingStrategy
 * works inside a directory.
 */
void JournalQueuingStrategyTest::setUp()
{
	FileTestFixture::setUpAsDirectory();
}

void JournalQueuingStrategyTest::doTestData(
	const vector<SensorData> &data,
	const string &raw,
	const string &hash) const
{
	ChecksumSensorDataParser parser(new JSONSensorDataParser);
	ChecksumSensorDataFormatter formatter(new JSONSensorDataFormatter);
	SafeWriter writer(Path(testingPath(), "check"));

	CPPUNIT_ASSERT_EQUAL(
		raw,
		[&]() -> string {
			string buffer;

			for (const auto one : data)
				buffer += formatter.format(one) + "\n";

			return buffer;
		}());

	writer.stream(true) << raw;
	const auto result = writer.finalize();
	CPPUNIT_ASSERT_EQUAL(
		hash,
		DigestEngine::digestToHex(result.first));

}

/**
 * @brief Test that the testing data are correct. This test is also helpful
 * when changing the test to quickly discover the new checksums and hashes.
 */
void JournalQueuingStrategyTest::testTestingData()
{
	doTestData(data_1e90a60, raw_1e90a60, "1e90a6059b538bb614b762d1f94203fafb3533d6");
	doTestData(data_fdd5085, raw_fdd5085, "fdd5085abed67887ce412239738352fc3ae3936f");
	doTestData(data_7f23d5f, raw_7f23d5f, "7f23d5f8aa61ea540c4af41b59381a054dc0601d");
}

/**
 * @brief Test that JournalQueuingStrategy setups a new empty repository properly.
 * This leads to an empty index file.
 */
void JournalQueuingStrategyTest::testSetupFromScratch()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(Path(testingPath(), "index"));
	CPPUNIT_ASSERT_DIR_EMPTY(testingFile());

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT(strategy.empty());

	CPPUNIT_ASSERT_FILE_EXISTS(Path(testingPath(), "index"));
}

/**
 * @brief Test that JournalQueuingStrategy setups properly when an empty index
 * already exists. The index should be preserved.
 */
void JournalQueuingStrategyTest::testSetupExistingEmpty()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());

	File index(Path(testingPath(), "index"));

	writeFile(index, "");
	CPPUNIT_ASSERT_FILE_EXISTS(index);

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT(strategy.empty());

	CPPUNIT_ASSERT_FILE_EXISTS(index);
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS("", index);
}

/**
 * @brief Test that JournalQueuingStrategy setups properly on an existing valid repository.
 * The index and the existing buffers should be untouched after setup.
 */
void JournalQueuingStrategyTest::testSetupExisting()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());

	File data0(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	writeFile(data0, raw_1e90a60);

	File data1(Path(testingPath(), "fdd5085abed67887ce412239738352fc3ae3936f"));
	writeFile(data1, raw_fdd5085);

	File index(Path(testingPath(), "index"));
	writeFile(index,
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(raw_1e90a60, data0);
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(raw_fdd5085, data1);
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n",
		index);
}

/**
 * @brief Test setup of JournalQueuingStrategy on repository with a broken buffer file.
 * After the setup (with GC disabled), the broken buffer must be marked dropped in the
 * index and the broken file must be removed.
 */
void JournalQueuingStrategyTest::testSetupWithBroken()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());

	File data0(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	writeFile(data0, raw_7f23d5f); // this will not match

	File data1(Path(testingPath(), "fdd5085abed67887ce412239738352fc3ae3936f"));
	writeFile(data1, raw_fdd5085);

	File index(Path(testingPath(), "index"));
	writeFile(index,
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT(!strategy.empty());

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n"
		"521E6294\t1e90a6059b538bb614b762d1f94203fafb3533d6\tdrop\n",
		index);
	CPPUNIT_ASSERT_FILE_NOT_EXISTS(data0);
	CPPUNIT_ASSERT_FILE_EXISTS(data1);
}

/**
 * @brief Test behaviour of a proper push() call into an empty repository.
 * After the push, the index should contain valid records and appropriate
 * buffers must exist. Multiple pushes appends to the index.
 */
void JournalQueuingStrategyTest::testPushSuccessful()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());

	File index(Path(testingPath(), "index"));

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(index);
	CPPUNIT_ASSERT_DIR_EMPTY(testingFile());

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());

	CPPUNIT_ASSERT_NO_THROW(strategy.push(data_1e90a60));

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n",
		index);

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		raw_1e90a60,
		Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));

	CPPUNIT_ASSERT_NO_THROW(strategy.push(data_fdd5085));

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		raw_1e90a60,
		Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		raw_fdd5085,
		Path(testingPath(), "fdd5085abed67887ce412239738352fc3ae3936f"));

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n",
		index);
}

/**
 * @brief Test push behaviour in case when the repository is not
 * writable (sign of an invalid disk state, ro-mounted disk, etc.).
 * Both setup() and push() must fail.
 */
void JournalQueuingStrategyTest::testPushNotWritable()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());

	File index(Path(testingPath(), "index"));

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(index);
	CPPUNIT_ASSERT_DIR_EMPTY(testingFile());

	testingFile().setReadOnly(true);
	CPPUNIT_ASSERT_THROW(strategy.setup(), FileAccessDeniedException);

	testingFile().setReadOnly(false);
	CPPUNIT_ASSERT_NO_THROW(strategy.setup());

	testingFile().setReadOnly(true);

	CPPUNIT_ASSERT_THROW(strategy.push(data_7f23d5f), FileAccessDeniedException);

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS("", index);
}

/**
 * @brief Test behaviour when the disk is full. We emulate this by creating
 * a symlink from the index file to /dev/full. Thus, we can only emulate such
 * behaviour whiel appending the index (wiring buffers would succeed).
 * When the push fails, the buffer would be written successfully, but the index
 * would remain untouched.
 */
void JournalQueuingStrategyTest::testPushDiskFullOnIndexAppend()
{
	File index(Path(testingPath(), "index"));
	CPPUNIT_ASSERT_NO_THROW(createLink("/dev/full", index));

	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());
	strategy.setDisableGC(true);

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());

	CPPUNIT_ASSERT_THROW(strategy.push(data_7f23d5f), WriteFileException);

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		raw_7f23d5f,
		Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));

	CPPUNIT_ASSERT(index.isLink());
	CPPUNIT_ASSERT_EQUAL(0, index.getSize());
}

/**
 * @brief Test situation when there is the data.tmp file in the repository while
 * pushing the exactly same data. We should not fail as we count that only
 * single JournalQueuingStrategy instance for the target directory is running.
 * The data.tmp file is created as a symlink to /dev/full to ensure that it is
 * not being written. The JournalQueuingStrategy must first delete it and
 * create a new file of such name.
 */
void JournalQueuingStrategyTest::testPushLockExists()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());

	File data(Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));
	File dataLock(Path(testingPath(), "data.tmp"));
	CPPUNIT_ASSERT_NO_THROW(createLink("/dev/full", dataLock));

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(data);
	CPPUNIT_ASSERT_FILE_EXISTS(dataLock);
	CPPUNIT_ASSERT(dataLock.isLink());

	CPPUNIT_ASSERT_NO_THROW(strategy.push(data_7f23d5f));

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(raw_7f23d5f, data);
	CPPUNIT_ASSERT_FILE_NOT_EXISTS(dataLock);
}

/**
 * @brief Test behaviour of JournalQueuingStrategy when pushing data over the
 * set bytes limit. As the garbage-collection is enabled in this test and there
 * is a dangling buffer in the repository, the push would succeed by whiping
 * that dangling buffer away.
 */
void JournalQueuingStrategyTest::testPushOverSizeWithGC()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());
	strategy.setDisableGC(false);
	strategy.setBytesLimit(600);

	File data(Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));
	writeFile(data, raw_7f23d5f);

	File dangling(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	writeFile(dangling, raw_1e90a60);

	File index(Path(testingPath(), "index"));
	writeFile(index,
		"24D75BA2\t7f23d5f8aa61ea540c4af41b59381a054dc0601d\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT_NO_THROW(strategy.push(data_263eb6d));

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(dangling);
	CPPUNIT_ASSERT_FILE_EXISTS(
		Path(testingPath(), "263eb6d629af44561aed6476e6d35eb0ad6bb493"));
}

/**
 * @brief Test behaviour of JournalQueuingStrategy in case when the set bytes
 * limit is to be exceeded while the garbage-collection is disabled. We however
 * has some untouched buffers that can be dropped on behalf of the newest data.
 * Dangling buffer is left untouched.
 */
void JournalQueuingStrategyTest::testPushOverSizeNoGC()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());
	strategy.setDisableGC(true);
	strategy.setBytesLimit(600);

	File data(Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));
	writeFile(data, raw_7f23d5f);

	File dangling(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	writeFile(dangling, raw_1e90a60);

	File index(Path(testingPath(), "index"));
	writeFile(index,
		"24D75BA2\t7f23d5f8aa61ea540c4af41b59381a054dc0601d\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"24D75BA2\t7f23d5f8aa61ea540c4af41b59381a054dc0601d\t0\n",
		index);

	CPPUNIT_ASSERT_NO_THROW(strategy.push(data_263eb6d));

	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"24D75BA2\t7f23d5f8aa61ea540c4af41b59381a054dc0601d\t0\n"
		"C53FB14F\t7f23d5f8aa61ea540c4af41b59381a054dc0601d\tdrop\n"
		"EDDBABE8\t263eb6d629af44561aed6476e6d35eb0ad6bb493\t0\n",
		index);

	CPPUNIT_ASSERT_FILE_EXISTS(dangling);
	CPPUNIT_ASSERT_FILE_NOT_EXISTS(data);
	CPPUNIT_ASSERT_FILE_EXISTS(
		Path(testingPath(), "263eb6d629af44561aed6476e6d35eb0ad6bb493"));
}

/**
 * @brief Test pushing too big data in case that rlimit RLIMIT_FSIZE is set too low.
 * This shows behaviour of failed writes into the file system and partiaially emulates
 * broken disk. It is not possible to write anything properly, thus we must always
 * end up with an exception.
 */
void JournalQueuingStrategyTest::testPushOverRLimit()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());

	File data0(Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));
	writeFile(data0, raw_7f23d5f);

	File data1(Path(testingPath(), "fdd5085abed67887ce412239738352fc3ae3936f"));
	writeFile(data1, raw_fdd5085);

	File index(Path(testingPath(), "index"));
	writeFile(index,
		"24D75BA2\t7f23d5f8aa61ea540c4af41b59381a054dc0601d\t0\n"
		"24D75BA2\tgarbage that is quite long to oversize rlimit\t2011321\n"
		"24D75BA2\tgarbage that is quite long to oversize rlimit\tabe4321\n"
		"24D75BA2\tgarbage that is quite long to oversize rlimit\t10f1242\n"
		"24D75BA2\tgarbage that is quite long to oversize rlimit\t12344dd\n"
		"24D75BA2\tgarbage that is quite long to oversize rlimit\t12344dd\n"
		"24D75BA2\tgarbage that is quite long to oversize rlimit\t12344dd\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());

	CPPUNIT_ASSERT_NO_THROW(PosixSignal::ignore("SIGXFSZ"));

	// first, try to push too big data (401 B) over the limit
	CPPUNIT_ASSERT_NO_THROW(updateFileRlimit(400));

	CPPUNIT_ASSERT_THROW(strategy.push(data_1e90a60), WriteFileException);

	CPPUNIT_ASSERT_FILE_EXISTS(data0);
	CPPUNIT_ASSERT_FILE_EXISTS(data1);
	CPPUNIT_ASSERT_FILE_NOT_EXISTS(
		Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));

	// second, push successfully but without unsuccessful index update
	// (there is no detectable redundancy inside the index and thus it
	// cannot be shrinked)
	CPPUNIT_ASSERT_NO_THROW(updateFileRlimit(index.getSize()));

	CPPUNIT_ASSERT_THROW(strategy.push(data_1e90a60), WriteFileException);

	CPPUNIT_ASSERT_FILE_EXISTS(data0);
	CPPUNIT_ASSERT_FILE_EXISTS(data1);
	CPPUNIT_ASSERT_FILE_EXISTS(
		Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"24D75BA2\t7f23d5f8aa61ea540c4af41b59381a054dc0601d\t0\n"
		"24D75BA2\tgarbage that is quite long to oversize rlimit\t2011321\n"
		"24D75BA2\tgarbage that is quite long to oversize rlimit\tabe4321\n"
		"24D75BA2\tgarbage that is quite long to oversize rlimit\t10f1242\n"
		"24D75BA2\tgarbage that is quite long to oversize rlimit\t12344dd\n"
		"24D75BA2\tgarbage that is quite long to oversize rlimit\t12344dd\n"
		"24D75BA2\tgarbage that is quite long to oversize rlimit\t12344dd\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n",
		index);
}

/**
 * @brief Test repeated peek() calls without any pop(). Such calls should lead to
 * stable results over time (until application restart).
 */
void JournalQueuingStrategyTest::testRepeatedPeekStable()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());

	File data0(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	writeFile(data0, raw_1e90a60);

	File data1(Path(testingPath(), "fdd5085abed67887ce412239738352fc3ae3936f"));
	writeFile(data1, raw_fdd5085);

	File index(Path(testingPath(), "index"));
	writeFile(index,
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());

	vector<SensorData> data;
	CPPUNIT_ASSERT_EQUAL(0, strategy.peek(data, 0));

	CPPUNIT_ASSERT_EQUAL(1, strategy.peek(data, 1));
	CPPUNIT_ASSERT(data[0] == data_1e90a60[0]);

	data.clear();
	CPPUNIT_ASSERT_EQUAL(2, strategy.peek(data, 2));
	CPPUNIT_ASSERT(data[0] == data_1e90a60[0]);
	CPPUNIT_ASSERT(data[1] == data_1e90a60[1]);

	data.clear();
	CPPUNIT_ASSERT_EQUAL(3, strategy.peek(data, 3));
	CPPUNIT_ASSERT(data[0] == data_1e90a60[0]);
	CPPUNIT_ASSERT(data[1] == data_1e90a60[1]);
	CPPUNIT_ASSERT(data[2] == data_1e90a60[2]);

	data.clear();
	CPPUNIT_ASSERT_EQUAL(4, strategy.peek(data, 4));
	CPPUNIT_ASSERT(data[0] == data_1e90a60[0]);
	CPPUNIT_ASSERT(data[1] == data_1e90a60[1]);
	CPPUNIT_ASSERT(data[2] == data_1e90a60[2]);
	CPPUNIT_ASSERT(data[3] == data_fdd5085[0]);

	data.clear();
	CPPUNIT_ASSERT_EQUAL(5, strategy.peek(data, 5));
	CPPUNIT_ASSERT(data[0] == data_1e90a60[0]);
	CPPUNIT_ASSERT(data[1] == data_1e90a60[1]);
	CPPUNIT_ASSERT(data[2] == data_1e90a60[2]);
	CPPUNIT_ASSERT(data[3] == data_fdd5085[0]);
	CPPUNIT_ASSERT(data[4] == data_fdd5085[1]);

	data.clear();
	CPPUNIT_ASSERT_EQUAL(5, strategy.peek(data, 6));
	CPPUNIT_ASSERT(data[0] == data_1e90a60[0]);
	CPPUNIT_ASSERT(data[1] == data_1e90a60[1]);
	CPPUNIT_ASSERT(data[2] == data_1e90a60[2]);
	CPPUNIT_ASSERT(data[3] == data_fdd5085[0]);
	CPPUNIT_ASSERT(data[4] == data_fdd5085[1]);

	CPPUNIT_ASSERT_FILE_EXISTS(data0);
	CPPUNIT_ASSERT_FILE_EXISTS(data1);
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n",
		index);
}

/**
 * @brief Test pop() behaviour on an empty strategy. Such pop() operation
 * should have no side-effects.
 */
void JournalQueuingStrategyTest::testPopFromEmpty()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());

	File index(Path(testingPath(), "index"));

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(index);
	CPPUNIT_ASSERT_DIR_EMPTY(testingFile());

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT(strategy.empty());
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS("", index);

	CPPUNIT_ASSERT_NO_THROW(strategy.pop(1));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS("", index);
}

/**
 * @brief Test popping of zero elements. Such operation should just succeed
 * without touching index or any buffers. Dangling buffers should stay intact.
 */
void JournalQueuingStrategyTest::testPopZero()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());
	strategy.setDisableGC(true);

	File data0(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	writeFile(data0, raw_1e90a60);

	File data1(Path(testingPath(), "fdd5085abed67887ce412239738352fc3ae3936f"));
	writeFile(data1, raw_fdd5085);

	File dangling(Path(testingPath(), "7f23d5f8aa61ea540c4af41b59381a054dc0601d"));
	writeFile(dangling, raw_7f23d5f);

	File index(Path(testingPath(), "index"));
	writeFile(index,
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT(!strategy.empty());

	CPPUNIT_ASSERT_NO_THROW(strategy.pop(0));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n",
		index);
	CPPUNIT_ASSERT_FILE_EXISTS(dangling);
}

/**
 * @brief Pop multiple data at once. In this way, we simply mark all buffers
 * as dropped (reflected in index by few records). Buffer files are not immediatelly
 * dropped.
 */
void JournalQueuingStrategyTest::testPopAtOnce()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());

	File data0(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	writeFile(data0, raw_1e90a60);

	File data1(Path(testingPath(), "fdd5085abed67887ce412239738352fc3ae3936f"));
	writeFile(data1, raw_fdd5085);

	File index(Path(testingPath(), "index"));
	writeFile(index,
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT(!strategy.empty());

	CPPUNIT_ASSERT_NO_THROW(strategy.pop(5));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n"
		"521E6294\t1e90a6059b538bb614b762d1f94203fafb3533d6\tdrop\n"
		"907B493C\tfdd5085abed67887ce412239738352fc3ae3936f\tdrop\n",
		index);
	CPPUNIT_ASSERT(strategy.empty());

	CPPUNIT_ASSERT_FILE_EXISTS(data0);
	CPPUNIT_ASSERT_FILE_EXISTS(data1);
}

/**
 * @brief Drop data in multiple steps. We end up with all buffers empty
 * (reflected in index by multiple records with offset incremented appropriately).
 */
void JournalQueuingStrategyTest::testPopInSteps()
{
	JournalQueuingStrategy strategy;
	strategy.setRootDir(testingFile().path());
	strategy.setDisableGC(true);

	File data0(Path(testingPath(), "1e90a6059b538bb614b762d1f94203fafb3533d6"));
	writeFile(data0, raw_1e90a60);

	File data1(Path(testingPath(), "fdd5085abed67887ce412239738352fc3ae3936f"));
	writeFile(data1, raw_fdd5085);

	File index(Path(testingPath(), "index"));
	writeFile(index,
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n");

	CPPUNIT_ASSERT_NO_THROW(strategy.setup());
	CPPUNIT_ASSERT(!strategy.empty());

	CPPUNIT_ASSERT_NO_THROW(strategy.pop(1));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n"
		"394B3594\t1e90a6059b538bb614b762d1f94203fafb3533d6\tA5\n",
		index);
	CPPUNIT_ASSERT(!strategy.empty());

	CPPUNIT_ASSERT_NO_THROW(strategy.pop(3));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n"
		"394B3594\t1e90a6059b538bb614b762d1f94203fafb3533d6\tA5\n"
		"521E6294\t1e90a6059b538bb614b762d1f94203fafb3533d6\tdrop\n"
		"37BB3228\tfdd5085abed67887ce412239738352fc3ae3936f\t86\n",
		index);
	CPPUNIT_ASSERT(!strategy.empty());

	CPPUNIT_ASSERT_NO_THROW(strategy.pop(1));
	CPPUNIT_ASSERT_FILE_TEXTUAL_EQUALS(
		"60EE675A\t1e90a6059b538bb614b762d1f94203fafb3533d6\t0\n"
		"62B820C9\tfdd5085abed67887ce412239738352fc3ae3936f\t0\n"
		"394B3594\t1e90a6059b538bb614b762d1f94203fafb3533d6\tA5\n"
		"521E6294\t1e90a6059b538bb614b762d1f94203fafb3533d6\tdrop\n"
		"37BB3228\tfdd5085abed67887ce412239738352fc3ae3936f\t86\n"
		"907B493C\tfdd5085abed67887ce412239738352fc3ae3936f\tdrop\n",
		index);
	CPPUNIT_ASSERT(strategy.empty());
}

}
