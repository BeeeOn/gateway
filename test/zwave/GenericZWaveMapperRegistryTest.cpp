#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"
#include "zwave/GenericZWaveMapperRegistry.h"
#include "zwave/ZWaveNode.h"

using namespace Poco;
using namespace BeeeOn;

using CC = ZWaveNode::CommandClass;

namespace BeeeOn {

class GenericZWaveMapperRegistryTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(GenericZWaveMapperRegistryTest);
	CPPUNIT_TEST(testResolveNonQueriedNode);
	CPPUNIT_TEST(testResolveUnsupportedNode);
	CPPUNIT_TEST(testResolveTempSensor);
	CPPUNIT_TEST_SUITE_END();

public:
	void testResolveNonQueriedNode();
	void testResolveUnsupportedNode();
	void testResolveTempSensor();
};

CPPUNIT_TEST_SUITE_REGISTRATION(GenericZWaveMapperRegistryTest);

void GenericZWaveMapperRegistryTest::testResolveNonQueriedNode()
{
	GenericZWaveMapperRegistry registry;
	ZWaveNode node({0x1000, 120});
	node.add({CC::BATTERY, 0, 0});
	node.add({CC::SENSOR_MULTILEVEL, 1, 0});

	ZWaveMapperRegistry::Mapper::Ptr mapper = registry.resolve(node);
	CPPUNIT_ASSERT(mapper.isNull());
}

void GenericZWaveMapperRegistryTest::testResolveUnsupportedNode()
{
	GenericZWaveMapperRegistry registry;
	ZWaveNode node({0x1000, 120});
	node.add({CC::ALARM, 6, 0});
	node.add({CC::SENSOR_MULTILEVEL, 168, 0});
	node.setQueried(true);

	ZWaveMapperRegistry::Mapper::Ptr mapper = registry.resolve(node);
	CPPUNIT_ASSERT(!mapper.isNull());
	CPPUNIT_ASSERT(mapper->types().empty());
}

void GenericZWaveMapperRegistryTest::testResolveTempSensor()
{
	GenericZWaveMapperRegistry registry;
	ZWaveNode node({0x1000, 120});
	node.add({CC::BATTERY, 0, 0});
	node.add({CC::SENSOR_MULTILEVEL, 1, 0});
	node.setQueried(true);

	ZWaveMapperRegistry::Mapper::Ptr mapper = registry.resolve(node);
	CPPUNIT_ASSERT(!mapper.isNull());

	const auto types = mapper->types();
	CPPUNIT_ASSERT_EQUAL(2, types.size());

	auto it = types.begin();
	CPPUNIT_ASSERT(it != types.end());
	CPPUNIT_ASSERT_EQUAL(
		ModuleType::Type::TYPE_BATTERY,
		it->type().raw());

	++it;
	CPPUNIT_ASSERT_EQUAL(
		"temperature",
		it->type().toString());
	CPPUNIT_ASSERT_EQUAL(
		ModuleType::Type::TYPE_TEMPERATURE,
		it->type().raw());

	++it;
	CPPUNIT_ASSERT(it == types.end());
}

}
