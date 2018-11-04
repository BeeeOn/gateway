#include <cppunit/extensions/HelperMacros.h>

#include <Poco/File.h>

#include "cppunit/BetterAssert.h"
#include "cppunit/FileTestFixture.h"

#include "core/FilesystemDeviceCache.h"
#include "model/DevicePrefix.h"

using namespace Poco;

namespace BeeeOn {

class FilesystemDeviceCacheTest : public FileTestFixture {
	CPPUNIT_TEST_SUITE(FilesystemDeviceCacheTest);
	CPPUNIT_TEST(testPairUnpair);
	CPPUNIT_TEST(testPrepaired);
	CPPUNIT_TEST(testBatchPair);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();

	void testPairUnpair();
	void testNothingPrepaired();
	void testPrepaired();
	void testBatchPair();

private:
	FilesystemDeviceCache m_cache;
};

CPPUNIT_TEST_SUITE_REGISTRATION(FilesystemDeviceCacheTest);

void FilesystemDeviceCacheTest::setUp()
{
	setUpAsDirectory();

	m_cache.setCacheDir(testingPath().toString());
}

void FilesystemDeviceCacheTest::tearDown()
{
	// remove all created named mutexes
	for (const auto &prefix : DevicePrefix::all()) {
		File mutex("/tmp/" + prefix.toString() + ".mutex");

		try {
			mutex.remove();
		}
		catch (...) {}
	}
}

/**
 * @brief Test we can pair and unpair a device and such device can be
 * detected as paired. Paired device would always have a corresponding
 * file created in the filesystem.
 */
void FilesystemDeviceCacheTest::testPairUnpair()
{
	const DevicePrefix &VDEV = DevicePrefix::PREFIX_VIRTUAL_DEVICE;

	const Path vdev(testingPath(), "vdev");
	CPPUNIT_ASSERT_FILE_NOT_EXISTS(vdev);

	const Path a300000001020304(testingPath(), "vdev/0xa300000001020304");

	CPPUNIT_ASSERT(m_cache.paired(VDEV).empty());
	CPPUNIT_ASSERT(!m_cache.paired({0xa300000001020304}));

	m_cache.markPaired({0xa300000001020304});

	CPPUNIT_ASSERT_FILE_EXISTS(vdev);

	CPPUNIT_ASSERT_FILE_EXISTS(a300000001020304);

	CPPUNIT_ASSERT_EQUAL(1, m_cache.paired(VDEV).size());
	CPPUNIT_ASSERT(m_cache.paired({0xa300000001020304}));

	m_cache.markUnpaired({0xa300000001020304});
	CPPUNIT_ASSERT_FILE_NOT_EXISTS(a300000001020304);

	CPPUNIT_ASSERT(m_cache.paired(VDEV).empty());
	CPPUNIT_ASSERT(!m_cache.paired({0xa300000001020304}));
}

/**
 * @brief Test we can pre-pair a set of devices by creating appropriate files
 * in the filesystem. Only such pre-paired devices are marked as paired.
 */
void FilesystemDeviceCacheTest::testPrepaired()
{
	const DevicePrefix &VDEV = DevicePrefix::PREFIX_VIRTUAL_DEVICE;

	const Path vdev(testingPath(), "vdev");
	CPPUNIT_ASSERT_FILE_NOT_EXISTS(vdev);
	CPPUNIT_ASSERT_NO_THROW(File(vdev).createDirectories());

	const Path a3000000aaaaaaaa(testingPath(), "vdev/0xa3000000aaaaaaaa");
	const Path a3000000bbbbbbbb(testingPath(), "vdev/0xa3000000bbbbbbbb");

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(a3000000aaaaaaaa);
	CPPUNIT_ASSERT_NO_THROW(File(a3000000aaaaaaaa).createFile());

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(a3000000bbbbbbbb);
	CPPUNIT_ASSERT_NO_THROW(File(a3000000bbbbbbbb).createFile());

	CPPUNIT_ASSERT_EQUAL(2, m_cache.paired(VDEV).size());
	CPPUNIT_ASSERT(m_cache.paired({0xa3000000aaaaaaaa}));
	CPPUNIT_ASSERT(m_cache.paired({0xa3000000bbbbbbbb}));
	CPPUNIT_ASSERT(!m_cache.paired({0xa300000001020304}));
}

/**
 * @brief Test pairing as a batch process. All already paired devices
 * should be removed and only the given set is to be paired. The pairing
 * status is visible when watching the filesystem.
 */
void FilesystemDeviceCacheTest::testBatchPair()
{
	const DevicePrefix &VDEV = DevicePrefix::PREFIX_VIRTUAL_DEVICE;

	const Path vdev(testingPath(), "vdev");
	CPPUNIT_ASSERT_FILE_NOT_EXISTS(vdev);

	const Path a3000000aaaaaaaa(testingPath(), "vdev/0xa3000000aaaaaaaa");
	const Path a3000000bbbbbbbb(testingPath(), "vdev/0xa3000000bbbbbbbb");
	const Path a300000001020304(testingPath(), "vdev/0xa300000001020304");

	CPPUNIT_ASSERT(m_cache.paired(VDEV).empty());

	m_cache.markPaired(VDEV, {{0xa3000000aaaaaaaa}, {0xa3000000bbbbbbbb}});

	CPPUNIT_ASSERT_FILE_EXISTS(a3000000aaaaaaaa);
	CPPUNIT_ASSERT_FILE_EXISTS(a3000000bbbbbbbb);
	CPPUNIT_ASSERT_FILE_NOT_EXISTS(a300000001020304);

	CPPUNIT_ASSERT_EQUAL(2, m_cache.paired(VDEV).size());
	CPPUNIT_ASSERT(m_cache.paired({0xa3000000aaaaaaaa}));
	CPPUNIT_ASSERT(m_cache.paired({0xa3000000bbbbbbbb}));
	CPPUNIT_ASSERT(!m_cache.paired({0xa300000001020304}));

	m_cache.markPaired(VDEV, {{0xa300000001020304}});

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(a3000000aaaaaaaa);
	CPPUNIT_ASSERT_FILE_NOT_EXISTS(a3000000bbbbbbbb);
	CPPUNIT_ASSERT_FILE_EXISTS(a300000001020304);

	CPPUNIT_ASSERT_EQUAL(1, m_cache.paired(VDEV).size());
	CPPUNIT_ASSERT(!m_cache.paired({0xa3000000aaaaaaaa}));
	CPPUNIT_ASSERT(!m_cache.paired({0xa3000000bbbbbbbb}));
	CPPUNIT_ASSERT(m_cache.paired({0xa300000001020304}));

	m_cache.markPaired(VDEV, {});
	CPPUNIT_ASSERT(m_cache.paired(VDEV).empty());

	CPPUNIT_ASSERT_FILE_NOT_EXISTS(a3000000aaaaaaaa);
	CPPUNIT_ASSERT_FILE_NOT_EXISTS(a3000000bbbbbbbb);
	CPPUNIT_ASSERT_FILE_NOT_EXISTS(a300000001020304);
	CPPUNIT_ASSERT_DIR_EMPTY(vdev);
}

}
