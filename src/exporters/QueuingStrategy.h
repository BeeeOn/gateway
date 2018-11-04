#pragma once

#include <string>
#include <vector>

#include <Poco/SharedPtr.h>

#include "model/SensorData.h"

namespace BeeeOn {

/**
 * @interface QueuingStrategy
 *
 * A class that implements this interface should provide holding a backup of SensorData. The typical usage
 * should be inserting, accessing and releasing the data.
 */
class QueuingStrategy {
public:
	typedef Poco::SharedPtr<QueuingStrategy> Ptr;

	virtual ~QueuingStrategy();

	/**
	 * @return True if no data are being held on the strategy.
	 */
	virtual bool empty() = 0;

	/**
	 * @brief Serves to insert data into the strategy.
	 * @param data Data to be inserted.
	 */
	virtual void push(const std::vector<SensorData> &data) = 0;

	/**
	 * @brief Serves to access data held by the strategy.
	 * @param data Vector to be filled with the data.
	 * @param count Required data count.
	 * @return The real count of the peeked data. This should match with the required count, if enough data are
	 *         available, or less, if there are less data available then required.
	 */
	virtual size_t peek(std::vector<SensorData> &data, size_t count) = 0;

	/**
	 * @brief Serves to release the data from the strategy.
	 *
	 * This method access the held data same way as the method peek(), so the same data that would be peeked by
	 * calling QueuingStrategy::peek(*count*) are released from the strategy by calling pop(*count*).
	 *
	 * @param count Count of data to be released.
	 */
	virtual void pop(size_t count) = 0;
};

}
