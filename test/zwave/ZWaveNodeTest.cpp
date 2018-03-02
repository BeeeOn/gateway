#include <cppunit/extensions/HelperMacros.h>

#include "zwave/ZWaveNode.h"

using namespace std;

namespace BeeeOn {

class ZWaveNodeTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(ZWaveNodeTest);
	CPPUNIT_TEST(testLessThan);
	CPPUNIT_TEST(testCommandClassLessThan);
	CPPUNIT_TEST_SUITE_END();
public:
	void testLessThan();
	void testCommandClassLessThan();
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

}
