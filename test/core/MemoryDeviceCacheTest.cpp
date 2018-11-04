#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"

#include "core/MemoryDeviceCache.h"

using namespace Poco;

namespace BeeeOn {

class MemoryDeviceCacheTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(MemoryDeviceCacheTest);
	CPPUNIT_TEST(testPairUnpair);
	CPPUNIT_TEST(testPrepaired);
	CPPUNIT_TEST(testBatchPair);
	CPPUNIT_TEST_SUITE_END();

public:
	void testPairUnpair();
	void testNothingPrepaired();
	void testPrepaired();
	void testBatchPair();
};

CPPUNIT_TEST_SUITE_REGISTRATION(MemoryDeviceCacheTest);

/**
 * @brief Test we can pair and unpair a device and such device can be
 * detected as paired.
 */
void MemoryDeviceCacheTest::testPairUnpair()
{
	MemoryDeviceCache cache;
	const DevicePrefix &VDEV = DevicePrefix::PREFIX_VIRTUAL_DEVICE;

	CPPUNIT_ASSERT(cache.paired(VDEV).empty());
	CPPUNIT_ASSERT(!cache.paired({0xa300000001020304}));

	cache.markPaired({0xa300000001020304});

	CPPUNIT_ASSERT_EQUAL(1, cache.paired(VDEV).size());
	CPPUNIT_ASSERT(cache.paired({0xa300000001020304}));

	cache.markUnpaired({0xa300000001020304});

	CPPUNIT_ASSERT(cache.paired(VDEV).empty());
	CPPUNIT_ASSERT(!cache.paired({0xa300000001020304}));
}

/**
 * @brief Test we can pre-pair a list of devices given as list of device IDs
 * expressed as strings. Only such pre-paired devices are marked as paired.
 */
void MemoryDeviceCacheTest::testPrepaired()
{
	MemoryDeviceCache cache;
	const DevicePrefix &VDEV = DevicePrefix::PREFIX_VIRTUAL_DEVICE;

	cache.setPrepaired({"0xa3000000aaaaaaaa", "0xa3000000bbbbbbbb"});

	CPPUNIT_ASSERT_EQUAL(2, cache.paired(VDEV).size());
	CPPUNIT_ASSERT(cache.paired({0xa3000000aaaaaaaa}));
	CPPUNIT_ASSERT(cache.paired({0xa3000000bbbbbbbb}));
	CPPUNIT_ASSERT(!cache.paired({0xa300000001020304}));
}

/**
 * @brief Test pairing as a batch process. All already paired devices
 * should be removed and only the given set is to be paired.
 */
void MemoryDeviceCacheTest::testBatchPair()
{
	MemoryDeviceCache cache;
	const DevicePrefix &VDEV = DevicePrefix::PREFIX_VIRTUAL_DEVICE;

	CPPUNIT_ASSERT(cache.paired(VDEV).empty());

	cache.markPaired(VDEV, {{0xa3000000aaaaaaaa}, {0xa3000000bbbbbbbb}});

	CPPUNIT_ASSERT_EQUAL(2, cache.paired(VDEV).size());
	CPPUNIT_ASSERT(cache.paired({0xa3000000aaaaaaaa}));
	CPPUNIT_ASSERT(cache.paired({0xa3000000bbbbbbbb}));
	CPPUNIT_ASSERT(!cache.paired({0xa300000001020304}));

	cache.markPaired(VDEV, {{0xa300000001020304}});

	CPPUNIT_ASSERT_EQUAL(1, cache.paired(VDEV).size());
	CPPUNIT_ASSERT(!cache.paired({0xa3000000aaaaaaaa}));
	CPPUNIT_ASSERT(!cache.paired({0xa3000000bbbbbbbb}));
	CPPUNIT_ASSERT(cache.paired({0xa300000001020304}));

	cache.markPaired(VDEV, {});
	CPPUNIT_ASSERT(cache.paired(VDEV).empty());
}

}
