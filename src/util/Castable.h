#ifndef BEEEON_TYPE_UTIL_H
#define BEEEON_TYPE_UTIL_H

#include <typeinfo>

namespace BeeeOn {

class Castable {
public:
	template <typename T>
	bool is() const
	{
		return typeid(*this) == typeid(T);
	}

	template<typename C>
	const C &cast() const
	{
		return dynamic_cast<const C &>(*this);
	}

	template<typename C>
	C &cast()
	{
		return dynamic_cast<C &>(*this);
	}

protected:
	virtual ~Castable()
	{
	}
};

}
#endif
