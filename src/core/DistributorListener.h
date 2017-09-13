#pragma once

#include <Poco/SharedPtr.h>

namespace BeeeOn {

class SensorData;

/**
 * @brief Interface to report events occuring in Distributor.
 *
 * Implement the DistributorListener and register it with the Distributor
 * used in the application to receive events about distribution.
 */
class DistributorListener
{
public:
	typedef Poco::SharedPtr<DistributorListener> Ptr;

	DistributorListener();
	virtual ~DistributorListener();

	/**
	 * @brief Event notifying about data being distributed
	 * @param data
	 * This method is called before the Distributor starts
	 * exporting the given data.
	 */
	virtual void onExport(const SensorData &data) = 0;
};

}
