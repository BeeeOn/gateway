#include <cppunit/extensions/HelperMacros.h>

#include "bluetooth/GlibPtr.h"

using namespace Poco;
using namespace std;

namespace BeeeOn {

class GlibPtrTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(GlibPtrTest);
	CPPUNIT_TEST(testGObject);
	CPPUNIT_TEST(testGMainLoop);
	CPPUNIT_TEST(testGError);
	CPPUNIT_TEST(testGList);
	CPPUNIT_TEST(testGVariant);
	CPPUNIT_TEST_SUITE_END();
public:
	void testGObject();
	void testGMainLoop();
	void testGError();
	void testGList();
	void testGVariant();
};

CPPUNIT_TEST_SUITE_REGISTRATION(GlibPtrTest);

void GlibPtrTest::testGObject()
{
	GlibPtr<GObject> object1;
	CPPUNIT_ASSERT_EQUAL(object1.isNull(), true);
	CPPUNIT_ASSERT_THROW(*object1, NullPointerException);
	CPPUNIT_ASSERT_THROW(object1.raw(), NullPointerException);

	object1 = reinterpret_cast<GObject*>(::g_object_new(G_TYPE_OBJECT, 0));
	CPPUNIT_ASSERT_EQUAL(object1.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*object1);
	CPPUNIT_ASSERT_NO_THROW(object1.raw());

	GlibPtr<GObject> object2 = object1;
	CPPUNIT_ASSERT_EQUAL(object2.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*object2);
	CPPUNIT_ASSERT_NO_THROW(object2.raw());

	GlibPtr<GObject> object3 = GlibPtr<GObject>(
		reinterpret_cast<GObject*>(::g_object_new(G_TYPE_OBJECT, 0)));
	CPPUNIT_ASSERT_EQUAL(object3.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*object3);
	CPPUNIT_ASSERT_NO_THROW(object3.raw());
}

void GlibPtrTest::testGMainLoop()
{
	GlibPtr<GMainLoop> loop1;
	CPPUNIT_ASSERT_EQUAL(loop1.isNull(), true);
	CPPUNIT_ASSERT_THROW(*loop1, NullPointerException);
	CPPUNIT_ASSERT_THROW(loop1.raw(), NullPointerException);

	loop1 = ::g_main_loop_new(nullptr, false);
	CPPUNIT_ASSERT_EQUAL(loop1.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*loop1);
	CPPUNIT_ASSERT_NO_THROW(loop1.raw());

	GlibPtr<GMainLoop> loop2 = loop1;
	CPPUNIT_ASSERT_EQUAL(loop2.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*loop2);
	CPPUNIT_ASSERT_NO_THROW(loop2.raw());

	GlibPtr<GMainLoop> loop3 = GlibPtr<GMainLoop>(::g_main_loop_new(nullptr, false));
	CPPUNIT_ASSERT_EQUAL(loop3.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*loop3);
	CPPUNIT_ASSERT_NO_THROW(loop3.raw());
}

void GlibPtrTest::testGError()
{
	GlibPtr<GError> error1;
	CPPUNIT_ASSERT_EQUAL(error1.isNull(), true);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"GlibPtr operator *",
		*error1,
		NullPointerException);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"GlibPtr method get",
		error1.raw(),
		NullPointerException);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"GlibPtr operator ->",
		error1->message,
		NullPointerException);
	CPPUNIT_ASSERT_NO_THROW(&error1);

	error1 = ::g_error_new(1, 1, "error");
	CPPUNIT_ASSERT_EQUAL(error1.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*error1);
	CPPUNIT_ASSERT_NO_THROW(error1.raw());
	CPPUNIT_ASSERT_NO_THROW(error1->message);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"cannot dereference non-const non-null GlibPtr",
		&error1,
		IllegalStateException);

	GlibPtr<GError> error2 = error1;
	CPPUNIT_ASSERT_EQUAL(error2.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*error2);
	CPPUNIT_ASSERT_NO_THROW(error2.raw());
	CPPUNIT_ASSERT_NO_THROW(error2->message);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"cannot dereference non-const non-null GlibPtr",
		&error2,
		IllegalStateException);

	GlibPtr<GError> error3 = GlibPtr<GError>(::g_error_new(1, 1, "error"));
	CPPUNIT_ASSERT_EQUAL(error3.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*error3);
	CPPUNIT_ASSERT_NO_THROW(error3.raw());
	CPPUNIT_ASSERT_NO_THROW(error3->message);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"cannot dereference non-const non-null GlibPtr",
		&error3,
		IllegalStateException);
}

void GlibPtrTest::testGList()
{
	GlibPtr<GList> list1;
	CPPUNIT_ASSERT_EQUAL(list1.isNull(), true);
	CPPUNIT_ASSERT_THROW(*list1, NullPointerException);
	CPPUNIT_ASSERT_THROW(list1.raw(), NullPointerException);

	GList* rawList1 = nullptr;
	rawList1 = ::g_list_append(rawList1, ::g_object_new(G_TYPE_OBJECT, 0));
	rawList1 = ::g_list_append(rawList1, ::g_object_new(G_TYPE_OBJECT, 0));

	list1 = rawList1;
	CPPUNIT_ASSERT_EQUAL(list1.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*list1);
	CPPUNIT_ASSERT_NO_THROW(list1.raw());
	CPPUNIT_ASSERT_EQUAL(static_cast<int>(::g_list_length(list1.raw())), 2);

	GlibPtr<GList> list2 = list1;
	CPPUNIT_ASSERT_EQUAL(list2.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*list2);
	CPPUNIT_ASSERT_NO_THROW(list2.raw());
	CPPUNIT_ASSERT_EQUAL(static_cast<int>(::g_list_length(list2.raw())), 2);

	GList* rawList2 = nullptr;
	rawList2 = ::g_list_append(rawList2, ::g_object_new(G_TYPE_OBJECT, 0));
	rawList2 = ::g_list_append(rawList2, ::g_object_new(G_TYPE_OBJECT, 0));

	GlibPtr<GList> list3 = GlibPtr<GList>(rawList2);
	CPPUNIT_ASSERT_EQUAL(list3.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*list3);
	CPPUNIT_ASSERT_NO_THROW(list3.raw());
	CPPUNIT_ASSERT_EQUAL(static_cast<int>(::g_list_length(list3.raw())), 2);
}

void GlibPtrTest::testGVariant()
{
	GlibPtr<GVariant> variant1;
	CPPUNIT_ASSERT_EQUAL(variant1.isNull(), true);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"GlibPtr operator *",
		*variant1,
		NullPointerException);
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"GlibPtr method get",
		variant1.raw(),
		NullPointerException);
	CPPUNIT_ASSERT_NO_THROW(&variant1);

	variant1 = ::g_variant_new("u", 40);
	CPPUNIT_ASSERT_EQUAL(variant1.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*variant1);
	CPPUNIT_ASSERT_NO_THROW(variant1.raw());
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"cannot dereference non-const non-null GlibPtr",
		&variant1,
		IllegalStateException);

	GlibPtr<GVariant> variant2 = variant1;
	CPPUNIT_ASSERT_EQUAL(variant2.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*variant2);
	CPPUNIT_ASSERT_NO_THROW(variant2.raw());
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"cannot dereference non-const non-null GlibPtr",
		&variant2,
		IllegalStateException);

	GlibPtr<GVariant> variant3 = GlibPtr<GVariant>(::g_variant_new("u", 40));
	CPPUNIT_ASSERT_EQUAL(variant3.isNull(), false);
	CPPUNIT_ASSERT_NO_THROW(*variant3);
	CPPUNIT_ASSERT_NO_THROW(variant3.raw());
	CPPUNIT_ASSERT_THROW_MESSAGE(
		"cannot dereference non-const non-null GlibPtr",
		&variant3,
		IllegalStateException);
}

}
