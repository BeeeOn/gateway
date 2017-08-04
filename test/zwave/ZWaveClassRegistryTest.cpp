#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"
#include "zwave/ZWaveClassRegistry.h"

using namespace Poco;

namespace BeeeOn {

class ZWaveClassRegistryTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ZWaveClassRegistryTest);
	CPPUNIT_TEST(testCommonCommandClass);
	CPPUNIT_TEST(testProductCommandClass);
	CPPUNIT_TEST_SUITE_END();

public:
	void testCommonCommandClass();
	void testProductCommandClass();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ZWaveClassRegistryTest);

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
		InvalidArgumentException
	);
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
