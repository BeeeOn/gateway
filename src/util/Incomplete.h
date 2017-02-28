#ifndef BEEEON_INCOMPLETE_H
#define BEEEON_INCOMPLETE_H

namespace BeeeOn {

/**
 * Any value, that can be in state "incomplete" for some time and
 * then completed by supplying some value, can be wrapped by this
 * template to avoid polluting of its code by the solving the completeness.
 */
template <typename T, typename CompleteTest, typename Complete>
class Incomplete {
public:
	/**
	 * Construct an implicit value of T, thus T is expected to have
	 * an empty constructor.
	 */
	Incomplete()
	{
	}

	Incomplete(const T &value):
		m_value(value)
	{
	}

	virtual ~Incomplete()
	{
	}

	/**
	 * Test whether the given value is complete. Utilize the template
	 * argument CompleteTest providing operator (const T &) for testing
	 * the completeness.
	 */
	bool isComplete() const
	{
		CompleteTest test;
		return test(m_value);
	}

	T deriveComplete(const Complete &complete) const
	{
		if (isComplete())
			return value();

		return complete(value());
	}

	Incomplete &completeSelf(const Complete &complete)
	{
		return assign(deriveComplete(complete));
	}

	T &value()
	{
		return m_value;
	}

	const T &value() const
	{
		return m_value;
	}

	operator T &()
	{
		return value();
	}

	operator const T &() const
	{
		return value();
	}

	Incomplete &assign(const T &value)
	{
		m_value = value;
		return *this;
	}

	Incomplete &assign(const Incomplete &other)
	{
		return assign(other.value());
	}

	Incomplete &operator =(const Incomplete &other)
	{
		return assign(other);
	}

	Incomplete &operator =(const T &other)
	{
		return assign(other);
	}

	bool operator ==(const Incomplete &other) const
	{
		if (isComplete() != other.isComplete())
			return false;

		return other.value() == value();
	}

	bool operator ==(const T &other) const
	{
		if (!isComplete())
			return false;

		return value() == other;
	}

	bool operator !=(const Incomplete &other) const
	{
		if (isComplete() != other.isComplete())
			return true;

		return other.value() != value();
	}

	bool operator !=(const T &other) const
	{
		if (!isComplete())
			return true;

		return value() != other;
	}

	bool operator <(const Incomplete &other) const
	{
		if (isComplete() == other.isComplete())
			return value() < other.value();

		if (!isComplete() && other.isComplete())
			return true;

		return false;
	}

	bool operator <(const T &other) const
	{
		if (isComplete())
			return value() < other;

		return true;
	}

	bool operator >(const Incomplete &other) const
	{
		return !(*this == other) && !(*this < other);
	}

private:
	T m_value;
};

}

#endif
