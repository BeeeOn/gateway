#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"

#include "core/QueuingExporter.h"
#include "exporters/QueuingStrategy.h"
#include "exporters/InMemoryQueuingStrategy.h"

using namespace std;
using namespace BeeeOn;
using namespace Poco;

namespace BeeeOn {
class TestableQueuingExporter : public QueuingExporter {
public:
	using QueuingExporter::acquire;
	using QueuingExporter::ack;
	using QueuingExporter::reset;
};

class TestingQueuingStrategyEmpty : public QueuingStrategy {
public:
	bool empty() override
	{
		return true;
	}

	void push(const vector<SensorData> &) override
	{
	}

	size_t peek(vector<SensorData> &, size_t ) override
	{
		return 0;
	}

	void pop(size_t ) override
	{
	}
};

class TestingQueuingStrategyInfinite : public QueuingStrategy {
public:
	TestingQueuingStrategyInfinite()
	{
		sensorData.setDeviceID(DeviceID(0x1111222233334444));
		sensorData.insertValue(SensorValue(ModuleID(3), 152));
	}

	bool empty() override
	{
		return false;
	}

	void push(const vector <SensorData> &) override
	{
	}

	size_t peek(vector <SensorData> &data, size_t count) override
	{
		for (size_t i = 0; i < count; ++i)
			data.emplace_back(sensorData);

		return count;
	}

	void pop(size_t) override
	{
	}

	const SensorData &data()
	{
		return sensorData;
	}

private:
	SensorData sensorData;
};

class TestingQueuingStrategyFailing : public QueuingStrategy {
public:
	bool empty() override
	{
		return false;
	}

	void push(const vector<SensorData>&) override
	{
		throw DataException();
	}

	size_t peek(vector<SensorData>&, size_t) override
	{
		throw DataException();
	}

	void pop(size_t) override
	{
		throw DataException();
	}
};

class QueuingExporterTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(QueuingExporterTest);
	CPPUNIT_TEST(testAcquireAck);
	CPPUNIT_TEST(testAcquireStable);
	CPPUNIT_TEST(testPushToStrategy);
	CPPUNIT_TEST(testPeekFromEmpty);
	CPPUNIT_TEST(testPeekFromFull);
	CPPUNIT_TEST(testEraseFromStrategy);
	CPPUNIT_TEST(testStrategyPriorityBasic);
	CPPUNIT_TEST(testStrategyPriorityEmptyStrategy);
	CPPUNIT_TEST(testStrategyPriorityEmptyExporter);
	CPPUNIT_TEST(testFailingStrategy);
	CPPUNIT_TEST_SUITE_END();

public:
	void testAcquireAck();
	void testAcquireStable();
	void testPushToStrategy();
	void testPeekFromEmpty();
	void testPeekFromFull();
	void testEraseFromStrategy();
	void testStrategyPriorityBasic();
	void testStrategyPriorityEmptyStrategy();
	void testStrategyPriorityEmptyExporter();
	void testFailingStrategy();
};

CPPUNIT_TEST_SUITE_REGISTRATION(QueuingExporterTest);

/**
 * The test verifies that the data given to the QueuingExporter via method ship()
 * are available via method acquire. Test also verifies the behavior of the method
 * ack, which is expected to erase acquired data.
 */
void QueuingExporterTest::testAcquireAck()
{
	TestableQueuingExporter exporter;
	QueuingStrategy::Ptr strategy = new InMemoryQueuingStrategy;
	exporter.setStrategy(strategy);
	exporter.setSaveThreshold(50);

	const SensorData testData1 = {
			0x8888999988889999,
			Timestamp(),
			{{44, 789}}
	};

	// 10 equal SensorData (testData1) are shipped via QueuingExporter
	for (int i = 0; i < 10; ++i)
		exporter.ship(testData1);

	const SensorData testData2 = {
			0x8888999988880000,
			Timestamp(),
			{{4, 79}}
	};

	// than another 2 equal SensorData (testData2), so now QueuingExporter
	// contains 10 of testData1 and 2 of testData2
	for (int i = 0; i < 2; ++i)
		exporter.ship(testData2);

	// saveThreshold is set to 50 and QueuingExporter contains only 12 SensorData
	// so its QueuingStrategy stays empty
	CPPUNIT_ASSERT(strategy->empty());

	vector<SensorData> vector;

	// we ask the QueuingExporter for a bunch of 10 SensorData and we test that
	// all of them are equal to testData1, as we shipped them first
	exporter.acquire(vector, 10, 0);
	CPPUNIT_ASSERT_EQUAL(10, vector.size());

	for (const auto &data : vector)
		CPPUNIT_ASSERT(testData1 == data);

	// now we acknowledge the successful export and ask for another bunch of
	// 10 SensorData, but the QueuingExporter should now contain only 2 of them
	// and they should be equal to the testData2, as we shipped them after
	// 10 testData1
	exporter.ack();
	vector.clear();
	exporter.acquire(vector, 10, 0);
	CPPUNIT_ASSERT_EQUAL(2, vector.size());

	for (const auto &data : vector)
		CPPUNIT_ASSERT(testData2 == data);

	// once again we acknowledge the successful export and ask for another bunch
	// of 10 SensorData, but the QueuingExporter should now be empty
	exporter.ack();
	vector.clear();
	exporter.acquire(vector, 10, 0);
	CPPUNIT_ASSERT_EQUAL(0, vector.size());
}
/**
* The test verifies that as long as the method ack() is not called,
* the behaviour of the method acquire() does not change.
*/
void QueuingExporterTest::testAcquireStable()
{
	TestableQueuingExporter exporter;
	QueuingStrategy::Ptr strategy = new InMemoryQueuingStrategy;
	exporter.setStrategy(strategy);
	exporter.setSaveThreshold(50);

	const SensorData testData1 = {
			0x8888999988889999,
			Timestamp(),
			{{44, 789}}
	};

	for (int i = 0; i < 5; ++i)
		exporter.ship(testData1);

	const SensorData testData2 = {
			0x8888999988880000,
			Timestamp(),
			{{4, 79}}
	};

	for (int i = 0; i < 5; ++i)
		exporter.ship(testData2);

	CPPUNIT_ASSERT(strategy->empty());

	vector<SensorData> vector;

	exporter.acquire(vector, 5, 0);
	CPPUNIT_ASSERT_EQUAL(5, vector.size());

	for (const auto &data : vector)
		CPPUNIT_ASSERT(testData1 == data);

	vector.clear();
	exporter.acquire(vector, 5, 0);
	CPPUNIT_ASSERT_EQUAL(5, vector.size());

	for (const auto &data : vector)
		CPPUNIT_ASSERT(testData1 == data);
}

/**
 * The test verifies, that when the saveThreshold is reached, QueuingExporter pushes
 * data to its QueuingStrategy.
 */
void QueuingExporterTest::testPushToStrategy()
{
	TestableQueuingExporter exporter;
	QueuingStrategy::Ptr strategy = new InMemoryQueuingStrategy;
	exporter.setStrategy(strategy);

	const SensorData testData = {
			0x8888999988889999,
			Timestamp(),
			{{44, 789}}
	};

	exporter.setSaveThreshold(5);

	CPPUNIT_ASSERT(strategy->empty());

	for (int i = 0; i < 4; ++i)
		exporter.ship(testData);

	CPPUNIT_ASSERT(strategy->empty());

	exporter.ship(testData);

	CPPUNIT_ASSERT(!strategy->empty());
	CPPUNIT_ASSERT_EQUAL(5, strategy.cast<InMemoryQueuingStrategy>()->size());
}

/**
 * The test verifies that when there are no data in the QueuingStrategy, nor shipped
 * via the QueuingExporter, the QueuingExporter does not provide any data
 */
void QueuingExporterTest::testPeekFromEmpty()
{
	TestableQueuingExporter exporter;
	QueuingStrategy::Ptr strategyEmpty = new TestingQueuingStrategyEmpty();

	exporter.setStrategy(strategyEmpty);
	vector<SensorData> vector;

	exporter.acquire(vector, 10, 0);
	CPPUNIT_ASSERT_EQUAL(0, vector.size());
}

/**
 * The test verifies that the data stored in the QueuingStrategy are accessible
 * via QueuingExporter.
 */
void QueuingExporterTest::testPeekFromFull()
{
	TestableQueuingExporter exporter;
	QueuingStrategy::Ptr strategyFull = new TestingQueuingStrategyInfinite();

	exporter.setStrategy(strategyFull);
	vector<SensorData> vector;

	exporter.acquire(vector, 10, 0);
	CPPUNIT_ASSERT_EQUAL(10, vector.size());
}

/**
 * The test verifies, that the method ack erases also the data acquired from the
 * QueuingStrategy via QueuingExporter.
 */
void QueuingExporterTest::testEraseFromStrategy()
{
	QueuingStrategy::Ptr strategy = new InMemoryQueuingStrategy;

	const SensorData testData = {
			0x8888999988889999,
			Timestamp(),
			{{44, 789}}
	};

	vector<SensorData> data;

	for (size_t i = 0; i < 20; ++i)
		data.emplace_back(testData);

	strategy->push(data);

	TestableQueuingExporter exporter;
	exporter.setStrategy(strategy);

	CPPUNIT_ASSERT_EQUAL(20, strategy.cast<InMemoryQueuingStrategy>()->size());

	vector<SensorData> vector;
	exporter.acquire(vector, 10, 0);

	CPPUNIT_ASSERT_EQUAL(10, vector.size());
	CPPUNIT_ASSERT_EQUAL(20, strategy.cast<InMemoryQueuingStrategy>()->size());

	exporter.ack();

	CPPUNIT_ASSERT_EQUAL(10, strategy.cast<InMemoryQueuingStrategy>()->size());
}

/**
* The test verifies that the ratio between the data from queuing strategy and
* the data in exporter internal buffer, provided by the method acquire(), matches
* the set backup priority.
*/
void QueuingExporterTest::testStrategyPriorityBasic()
{
	TestableQueuingExporter exporter;
	QueuingStrategy::Ptr strategy = new TestingQueuingStrategyInfinite();
	exporter.setStrategy(strategy);

	const SensorData testData = {
			0x8888999988889999,
			Timestamp(),
			{{44, 789}}
	};

	for (int i = 0; i < 6; ++i)
		exporter.ship(testData);

	exporter.setStrategyPriority(40);

	vector<SensorData> vector;

	exporter.acquire(vector, 10, 0);

	size_t queueData = 0;
	size_t strategyData = 0;

	for (const auto &data : vector) {
		if (data == testData)
			++queueData;

		if (data == strategy.cast<TestingQueuingStrategyInfinite>()->data())
			++strategyData;
	}

	CPPUNIT_ASSERT_EQUAL(6, queueData);
	CPPUNIT_ASSERT_EQUAL(4, strategyData);

	vector.clear();

	for (int i = 0; i < 10; ++i) {
		exporter.acquire(vector, 1, 0);
		exporter.ack();
	}

	queueData = 0;
	strategyData = 0;

	for (const auto &data : vector) {
		if (data == testData)
			++queueData;

		if (data == strategy.cast<TestingQueuingStrategyInfinite>()->data())
			++strategyData;
	}

	CPPUNIT_ASSERT_EQUAL(6, queueData);
	CPPUNIT_ASSERT_EQUAL(4, strategyData);
}

/**
 * The test verifies that when the QueuingStrategy does not enough data to provide,
 * the data are taken from the QueuingExporter buffer, despite the set strategyPriority.
 */
void QueuingExporterTest::testStrategyPriorityEmptyStrategy()
{
	TestableQueuingExporter exporter;
	QueuingStrategy::Ptr strategy = new TestingQueuingStrategyEmpty();
	exporter.setStrategy(strategy);

	const SensorData testData = {
			0x8888999988889999,
			Timestamp(),
			{{44, 789}}
	};

	for (int i = 0; i < 10; ++i)
		exporter.ship(testData);

	exporter.setStrategyPriority(40);

	vector<SensorData> vector;

	exporter.acquire(vector, 10, 0);

	CPPUNIT_ASSERT_EQUAL(10, vector.size());

	for (const auto &data : vector)
		CPPUNIT_ASSERT(data == testData);
}

/**
 * The test verifies that when the QueuingExporter buffer does not enough data to provide,
 * the data are taken from the QueuingStrategy, despite the set strategyPriority.
 */
void QueuingExporterTest::testStrategyPriorityEmptyExporter()
{
	TestableQueuingExporter exporter;
	QueuingStrategy::Ptr strategy = new TestingQueuingStrategyInfinite();
	exporter.setStrategy(strategy);

	exporter.setStrategyPriority(40);

	vector<SensorData> vector;

	exporter.acquire(vector, 10, 0);

	CPPUNIT_ASSERT_EQUAL(10, vector.size());
	for (const auto &data : vector)
		CPPUNIT_ASSERT(data == strategy.cast<TestingQueuingStrategyInfinite>()->data());
}

/**
* The test verifies that exceptions thrown by the QueuingStrategy methods
* peek, pop and ack are handled in the QueuingExporter.
*/
void QueuingExporterTest::testFailingStrategy()
{
	TestableQueuingExporter exporter;
	QueuingStrategy::Ptr strategy = new TestingQueuingStrategyFailing();

	exporter.setStrategy(strategy);
	exporter.setSaveThreshold(1);

	const SensorData testData = {
			0x8888999988889999,
			Timestamp(),
			{{44, 789}}
	};

	CPPUNIT_ASSERT_NO_THROW(exporter.ship(testData));

	vector<SensorData> vector;

	CPPUNIT_ASSERT_NO_THROW(exporter.acquire(vector, 10, 0));
	CPPUNIT_ASSERT_NO_THROW(exporter.ack());
}

}
