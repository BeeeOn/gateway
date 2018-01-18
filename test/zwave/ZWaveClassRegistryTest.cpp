#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"
#include "zwave/ZWaveClassRegistry.h"

using namespace Poco;

namespace BeeeOn {

class ZWaveClassRegistryTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ZWaveClassRegistryTest);
	CPPUNIT_TEST(testCompareZWaveCommandClassKey);
	CPPUNIT_TEST(testCommonCommandClass);
	CPPUNIT_TEST(testProductCommandClass);
	CPPUNIT_TEST_SUITE_END();

public:
	void testCompareZWaveCommandClassKey();
	void testCommonCommandClass();
	void testProductCommandClass();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ZWaveClassRegistryTest);

void ZWaveClassRegistryTest::testCompareZWaveCommandClassKey()
{
	ZWaveCommandClassKey a{0, 0};
	ZWaveCommandClassKey b{0, 1};
	ZWaveCommandClassKey c{1, 0};
	ZWaveCommandClassKey d{1, 1};

	CPPUNIT_ASSERT(!(a < a));
	CPPUNIT_ASSERT(a < b);
	CPPUNIT_ASSERT(a < c);
	CPPUNIT_ASSERT(a < d);

	CPPUNIT_ASSERT(!(b < a));
	CPPUNIT_ASSERT(!(b < b));
	CPPUNIT_ASSERT(b < c);
	CPPUNIT_ASSERT(b < d);

	CPPUNIT_ASSERT(!(c < a));
	CPPUNIT_ASSERT(!(c < b));
	CPPUNIT_ASSERT(!(c < c));
	CPPUNIT_ASSERT(c < d);

	CPPUNIT_ASSERT(!(d < a));
	CPPUNIT_ASSERT(!(d < b));
	CPPUNIT_ASSERT(!(d < c));
	CPPUNIT_ASSERT(!(d < d));
}

void ZWaveClassRegistryTest::testCommonCommandClass()
{
	ZWaveCommandClassMap emptyMap;
	ZWaveClassRegistry::Ptr zwaveRegistry(new ZWaveProductClassRegistry(emptyMap));

	CPPUNIT_ASSERT(zwaveRegistry->contains(49, 0x01));
	CPPUNIT_ASSERT(!zwaveRegistry->contains(0, 0x00));

	CPPUNIT_ASSERT_EQUAL(
		zwaveRegistry->find(49, 1).type(),
		ModuleType::Type::TYPE_TEMPERATURE
	);

	CPPUNIT_ASSERT_THROW(
		zwaveRegistry->find(0, 0).type(),
		NotFoundException
	);

	CPPUNIT_ASSERT(zwaveRegistry->contains(128, 0));
}

void ZWaveClassRegistryTest::testProductCommandClass()
{
	ZWaveCommandClassMap productMap = {
		{{200,0x00}, {ModuleType::Type::TYPE_TEMPERATURE}},
	};

	ZWaveClassRegistry::Ptr zwaveRegistry(new ZWaveProductClassRegistry(productMap));

	CPPUNIT_ASSERT(zwaveRegistry->contains(200, 0x00));
	CPPUNIT_ASSERT(!zwaveRegistry->contains(200, 0x01));

	CPPUNIT_ASSERT_EQUAL(
		zwaveRegistry->find(200, 0).type(),
		ModuleType::Type::TYPE_TEMPERATURE
	);
}

}
