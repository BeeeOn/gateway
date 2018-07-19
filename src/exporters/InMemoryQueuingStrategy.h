#pragma once

#include <vector>

#include <Poco/SharedPtr.h>

#include "exporters/QueuingStrategy.h"

namespace BeeeOn {

/**
 * @brief Basic implementation of the QueuingStrategy interface.
 *
 * Serves as temporary non-persistent storage of SensorData. The data are held
 * in the std::vector.
 */
class InMemoryQueuingStrategy: public QueuingStrategy {
public:
	typedef Poco::SharedPtr<InMemoryQueuingStrategy> Ptr;

	/**
	 * @return True, if the vector is empty, false otherwise.
	 */
	bool empty() override;

	/**
	 * @return actual size of the in-memory queue.
	 */
	size_t size();

	/**
	 * Adds the given data to the end of the vector.
	 * @param data
	 */
	void push(const std::vector<SensorData> &data) override;

	/**
	 * Peek the given count of data off the vector starting from the oldest one.
	 * Calling this method is stable (returns the same results) until the pop()
	 * method is called.
	 *
	 * @param data Vector to be filled with data
	 * @param count Required count of data
	 * @return Real count of data added to the "data" vector.
	 */
	size_t peek(std::vector<SensorData> &data, size_t count) override;

	/**
	 * Pop the required amount of data off the vector.
	 * @param count Number of data to be popped
	 */
	void pop(size_t count) override;

private:
	std::vector<SensorData> m_vector;
};

}
