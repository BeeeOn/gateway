#pragma once

#include <glib.h>
#include <gio/gio.h>

#include <utility>

#include <Poco/Exception.h>

#include "bluetooth/GlibPtr.h"

namespace BeeeOn {

static inline gpointer g_object_ref_copy(gconstpointer src, gpointer)
{
	return ::g_object_ref(const_cast<gpointer>(src));
}

/**
 * @brief The class is used to store the references to objects from the GLib library.
 * It is also responsible for automatically freeing of memory.
 *
 * In specialization for GList are used g_object_ref and g_object_unref for its items.
 */
template <typename T>
class GlibPtr {
public:
	GlibPtr();
	GlibPtr(T* ptr);
	GlibPtr(const GlibPtr& ptr);
	GlibPtr(GlibPtr&& ptr);
	~GlibPtr();

	GlibPtr& operator =(T* ptr);
	GlibPtr& operator =(const GlibPtr& ptr);
	GlibPtr& operator =(GlibPtr&& ptr);
	T* operator ->();
	const T* operator ->() const;
	T& operator *();
	const T& operator *() const;
	T* raw();
	const T* raw() const;

	/**
	 * @brief Returns reference of the m_ptr attribute.
	 * @throws IllegalStateException in case m_ptr is not null
	 */
	T** operator &();

	/**
	 * @brief Returns reference of the m_ptr attribute.
	 */
	const T** operator &() const;
	bool operator !() const;
	bool isNull() const;

protected:
	void release();
	void assign(T* ptr);

private:
	T* m_ptr;
};

template <typename T>
GlibPtr<T>::GlibPtr():
	m_ptr(nullptr)
{
}

template <typename T>
GlibPtr<T>::GlibPtr(T* ptr):
	m_ptr(ptr)
{
}

template <typename T>
GlibPtr<T>::GlibPtr(const GlibPtr& ptr):
	m_ptr(nullptr)
{
	assign(ptr.m_ptr);
}


template <typename T>
GlibPtr<T>::GlibPtr(GlibPtr&& ptr):
	m_ptr(nullptr)
{
	std::swap(m_ptr, ptr.m_ptr);
}

template <typename T>
GlibPtr<T>::~GlibPtr()
{
	release();
}

template <typename T>
GlibPtr<T>& GlibPtr<T>::operator =(T* ptr)
{
	return (*this = GlibPtr<T>{ptr});
}

template <typename T>
GlibPtr<T>& GlibPtr<T>::operator =(const GlibPtr& ptr)
{
	if (ptr.m_ptr != m_ptr)
		assign(ptr.m_ptr);

	return *this;
}

template <typename T>
GlibPtr<T>& GlibPtr<T>::operator =(GlibPtr&& ptr)
{
	std::swap(m_ptr, ptr.m_ptr);
	return *this;
}

template <typename T>
T* GlibPtr<T>::operator ->()
{
	if (isNull())
		throw Poco::NullPointerException(
			"GlibPtr operator ->");

	return m_ptr;
}

template <typename T>
const T* GlibPtr<T>::operator ->() const
{
	if (isNull())
		throw Poco::NullPointerException(
			"GlibPtr const operator ->");

	return m_ptr;
}

template <typename T>
T& GlibPtr<T>::operator *()
{
	if (isNull())
		throw Poco::NullPointerException(
			"GlibPtr operator *");

	return *m_ptr;
}

template <typename T>
const T& GlibPtr<T>::operator *() const
{
	if (isNull())
		throw Poco::NullPointerException(
			"GlibPtr const operator *");

	return *m_ptr;
}

template <typename T>
T* GlibPtr<T>::raw()
{
	if (isNull())
		throw Poco::NullPointerException(
			"GlibPtr method raw");

	return m_ptr;
}

template <typename T>
const T* GlibPtr<T>::raw() const
{
	if (isNull())
		throw Poco::NullPointerException(
			"GlibPtr method const raw");

	return m_ptr;
}

template <typename T>
T** GlibPtr<T>::operator &()
{
	if (isNull())
		return &m_ptr;

	throw Poco::IllegalStateException(
		"cannot dereference non-const non-null GlibPtr");
}

template <typename T>
const T** GlibPtr<T>::operator &() const
{
	return &m_ptr;
}

template <typename T>
bool GlibPtr<T>::operator !() const
{
	return isNull();
}

template <typename T>
bool GlibPtr<T>::isNull() const
{
	return m_ptr == nullptr;
}

template <typename GO>
inline void GlibPtr<GO>::release()
{
	g_clear_object(&m_ptr);
}

template <typename GO>
inline void GlibPtr<GO>::assign(GO* ptr)
{
	release();
	if (ptr != nullptr)
		m_ptr = reinterpret_cast<GO*>(::g_object_ref(ptr));
}

template <>
inline void GlibPtr<GMainLoop>::release()
{
	if (!isNull()) {
		::g_main_loop_unref(m_ptr);
		m_ptr = nullptr;
	}
}

template <>
inline void GlibPtr<GMainLoop>::assign(GMainLoop* ptr)
{
	release();
	if (ptr != nullptr)
		m_ptr = ::g_main_loop_ref(ptr);
}

template <>
inline void GlibPtr<GError>::release()
{
	::g_clear_error(&m_ptr);
}

template <>
inline void GlibPtr<GError>::assign(GError* ptr)
{
	release();
	if (ptr != nullptr)
		m_ptr = ::g_error_copy(ptr);
}

template <>
inline void GlibPtr<GList>::release()
{
	if (!isNull()) {
		::g_list_free_full(m_ptr, g_object_unref);
		m_ptr = nullptr;
	}
}

template <>
inline void GlibPtr<GList>::assign(GList* ptr)
{
	release();
	if (ptr != nullptr)
		m_ptr = ::g_list_copy_deep(ptr, g_object_ref_copy, nullptr);
}

template <>
inline void GlibPtr<GVariant>::release()
{
	if (!isNull()) {
		::g_variant_unref(m_ptr);
		m_ptr = nullptr;
	}
}

template <>
inline void GlibPtr<GVariant>::assign(GVariant* ptr)
{
	release();
	if (ptr != nullptr)
		m_ptr = ::g_variant_ref(ptr);
}


}
