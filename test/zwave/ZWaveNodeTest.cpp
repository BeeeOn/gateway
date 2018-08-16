#include <Poco/Exception.h>

#include <cppunit/extensions/HelperMacros.h>

#include "cppunit/BetterAssert.h"
#include "zwave/ZWaveNode.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class ZWaveNodeTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ZWaveNodeTest);
	CPPUNIT_TEST(testLessThan);
	CPPUNIT_TEST(testCommandClassLessThan);
	CPPUNIT_TEST(testValueAsBool);
	CPPUNIT_TEST(testValueAsHex32);
	CPPUNIT_TEST(testValueAsDouble);
	CPPUNIT_TEST(testValueAsInt);
	CPPUNIT_TEST(testValueAsCelsius);
	CPPUNIT_TEST(testValueAsLuminance);
	CPPUNIT_TEST(testValueAsPM25);
	CPPUNIT_TEST(testValueAsTime);
	CPPUNIT_TEST_SUITE_END();
public:
	using Value = ZWaveNode::Value;

	void testLessThan();
	void testCommandClassLessThan();
	void testValueAsBool();
	void testValueAsHex32();
	void testValueAsDouble();
	void testValueAsInt();
	void testValueAsCelsius();
	void testValueAsLuminance();
	void testValueAsPM25();
	void testValueAsTime();
};

CPPUNIT_TEST_SUITE_REGISTRATION(ZWaveNodeTest);

void ZWaveNodeTest::testLessThan()
{
	ZWaveNode a({0x1000, 0x0});
	ZWaveNode b({0x1000, 0x1});
	ZWaveNode c({0x1000, 0x2});
	ZWaveNode d({0x0100, 0x3});

	CPPUNIT_ASSERT(a < b);
	CPPUNIT_ASSERT(b < c);
	CPPUNIT_ASSERT(d < a);
	CPPUNIT_ASSERT(d < b);
	CPPUNIT_ASSERT(d < c);
	CPPUNIT_ASSERT(!(a < a));
	CPPUNIT_ASSERT(!(b < b));
	CPPUNIT_ASSERT(!(c < c));
	CPPUNIT_ASSERT(!(d < d));
	CPPUNIT_ASSERT(!(a < d));
}

void ZWaveNodeTest::testCommandClassLessThan()
{
	ZWaveNode::CommandClass a(10, 0, 0);
	ZWaveNode::CommandClass b(10, 1, 0);
	ZWaveNode::CommandClass c(10, 1, 1);
	ZWaveNode::CommandClass d(10, 2, 0);
	ZWaveNode::CommandClass e( 5, 3, 0);

	CPPUNIT_ASSERT(a < b);
	CPPUNIT_ASSERT(b < c);
	CPPUNIT_ASSERT(c < d);
	CPPUNIT_ASSERT(e < a);
	CPPUNIT_ASSERT(e < b);
	CPPUNIT_ASSERT(e < c);
	CPPUNIT_ASSERT(e < d);
	CPPUNIT_ASSERT(!(a < a));
	CPPUNIT_ASSERT(!(b < b));
	CPPUNIT_ASSERT(!(c < c));
	CPPUNIT_ASSERT(!(d < d));
	CPPUNIT_ASSERT(!(e < e));
}

void ZWaveNodeTest::testValueAsBool()
{
	CPPUNIT_ASSERT(Value({0, 0}, {0, 0, 0}, "true").asBool());
	CPPUNIT_ASSERT(Value({0, 0}, {0, 0, 0}, "True").asBool());
	CPPUNIT_ASSERT(Value({0, 0}, {0, 0, 0}, "1").asBool());

	CPPUNIT_ASSERT(!Value({0, 0}, {0, 0, 0}, "false").asBool());
	CPPUNIT_ASSERT(!Value({0, 0}, {0, 0, 0}, "False").asBool());
	CPPUNIT_ASSERT(!Value({0, 0}, {0, 0, 0}, "0").asBool());

	CPPUNIT_ASSERT(Value({0, 0}, {0, 0, 0}, "10").asBool());

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "11.021").asBool(),
		SyntaxException);
	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "something").asBool(),
		SyntaxException);
	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "").asBool(),
		SyntaxException);
}

void ZWaveNodeTest::testValueAsHex32()
{
	CPPUNIT_ASSERT_EQUAL(
		0x01234567,
		Value({0, 0}, {0, 0, 0}, "0x01234567").asHex32());

	CPPUNIT_ASSERT_EQUAL(
		0x01234567,
		Value({0, 0}, {0, 0, 0}, "01234567").asHex32());

	CPPUNIT_ASSERT_EQUAL(
		0xffffffff,
		Value({0, 0}, {0, 0, 0}, "0xffffffff").asHex32());

	CPPUNIT_ASSERT_EQUAL(
		0,
		Value({0, 0}, {0, 0, 0}, "0").asHex32());

	CPPUNIT_ASSERT_EQUAL(
		0x10,
		Value({0, 0}, {0, 0, 0}, "10").asHex32());

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "11.021").asHex32(),
		SyntaxException);

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "something").asHex32(),
		SyntaxException);

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "").asHex32(),
		SyntaxException);
}

void ZWaveNodeTest::testValueAsDouble()
{
	CPPUNIT_ASSERT_EQUAL(
		100.13,
		Value({0, 0}, {0, 0, 0}, "100.13").asDouble());

	CPPUNIT_ASSERT_EQUAL(
		100,
		Value({0, 0}, {0, 0, 0}, "100.0").asDouble());

	CPPUNIT_ASSERT_EQUAL(
		10,
		Value({0, 0}, {0, 0, 0}, "10").asDouble());

	CPPUNIT_ASSERT_EQUAL(
		0,
		Value({0, 0}, {0, 0, 0}, "0").asDouble());

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "11h9012").asDouble(),
		SyntaxException);

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "something").asDouble(),
		SyntaxException);

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "").asDouble(),
		SyntaxException);
}

void ZWaveNodeTest::testValueAsInt()
{
	CPPUNIT_ASSERT_EQUAL(
		1000,
		Value({0, 0}, {0, 0, 0}, "1000").asInt());

	CPPUNIT_ASSERT_EQUAL(
		-1000,
		Value({0, 0}, {0, 0, 0}, "-1000").asInt());

	CPPUNIT_ASSERT_EQUAL(
		-1,
		Value({0, 0}, {0, 0, 0}, "-1").asInt());

	CPPUNIT_ASSERT_EQUAL(
		1,
		Value({0, 0}, {0, 0, 0}, "+1").asInt());

	CPPUNIT_ASSERT_EQUAL(
		0,
		Value({0, 0}, {0, 0, 0}, "0").asInt());

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "0.1234").asInt(),
		SyntaxException);

	CPPUNIT_ASSERT_EQUAL(
		0,
		Value({0, 0}, {0, 0, 0}, "0.1234").asInt(true));

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "120.5").asInt(),
		SyntaxException);

	CPPUNIT_ASSERT_EQUAL(
		120,
		Value({0, 0}, {0, 0, 0}, "120.5").asInt(true));

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "1231l").asInt(),
		SyntaxException);

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "something").asInt(),
		SyntaxException);

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "").asInt(),
		SyntaxException);
}

void ZWaveNodeTest::testValueAsCelsius()
{
	CPPUNIT_ASSERT_EQUAL(
		100,
		Value({0, 0}, {0, 0, 0}, "100", "C").asCelsius());

	CPPUNIT_ASSERT_EQUAL(
		-15.0,
		Value({0, 0}, {0, 0, 0}, "-15", "C").asCelsius());

	CPPUNIT_ASSERT_EQUAL(
		2.5,
		Value({0, 0}, {0, 0, 0}, "2.5", "C").asCelsius());

	CPPUNIT_ASSERT_EQUAL(
		-12.8,
		Value({0, 0}, {0, 0, 0}, "-12.8", "C").asCelsius());

	CPPUNIT_ASSERT_EQUAL(
		20,
		Value({0, 0}, {0, 0, 0}, "68", "F").asCelsius());

	CPPUNIT_ASSERT_EQUAL(
		30,
		Value({0, 0}, {0, 0, 0}, "86", "F").asCelsius());

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "100").asCelsius(),
		InvalidArgumentException);

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "68").asCelsius(),
		InvalidArgumentException);

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "100", "K").asCelsius(),
		InvalidArgumentException);
}

void ZWaveNodeTest::testValueAsLuminance()
{
	CPPUNIT_ASSERT_EQUAL(
		10,
		Value({0, 0}, {0, 0, 0}, "10", "lux").asLuminance());

	CPPUNIT_ASSERT_EQUAL(
		0,
		Value({0, 0}, {0, 0, 0}, "0", "lux").asLuminance());

	CPPUNIT_ASSERT_EQUAL(
		14.5,
		Value({0, 0}, {0, 0, 0}, "14.5", "lux").asLuminance());

	CPPUNIT_ASSERT_EQUAL(
		0,
		Value({0, 0}, {0, 0, 0}, "0", "%").asLuminance());

	CPPUNIT_ASSERT_EQUAL(
		1000,
		Value({0, 0}, {0, 0, 0}, "100", "%").asLuminance());

	CPPUNIT_ASSERT_EQUAL(
		1000,
		Value({0, 0}, {0, 0, 0}, "200", "%").asLuminance());

	CPPUNIT_ASSERT_EQUAL(
		10,
		Value({0, 0}, {0, 0, 0}, "1", "%").asLuminance());

	CPPUNIT_ASSERT_EQUAL(
		100,
		Value({0, 0}, {0, 0, 0}, "10", "%").asLuminance());

	CPPUNIT_ASSERT_EQUAL(
		500,
		Value({0, 0}, {0, 0, 0}, "50", "%").asLuminance());

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "100", "").asLuminance(),
		InvalidArgumentException);

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "100", "lx").asLuminance(),
		InvalidArgumentException);
}

void ZWaveNodeTest::testValueAsPM25()
{
	CPPUNIT_ASSERT_EQUAL(
		10,
		Value({0, 0}, {0, 0, 0}, "10", "ug/m3").asPM25());

	CPPUNIT_ASSERT_EQUAL(
		15.5,
		Value({0, 0}, {0, 0, 0}, "15.5", "ug/m3").asPM25());

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "10", "").asPM25(),
		InvalidArgumentException);
}

void ZWaveNodeTest::testValueAsTime()
{
	CPPUNIT_ASSERT_EQUAL(
		10,
		Value({0, 0}, {0, 0, 0}, "10", "seconds").asTime().totalSeconds());

	CPPUNIT_ASSERT_EQUAL(
		3600,
		Value({0, 0}, {0, 0, 0}, "3600", "seconds").asTime().totalSeconds());

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "10", "").asTime(),
		InvalidArgumentException);

	CPPUNIT_ASSERT_THROW(
		Value({0, 0}, {0, 0, 0}, "10", "hours").asTime(),
		InvalidArgumentException);
}

}
