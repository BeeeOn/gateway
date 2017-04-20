#pragma once

namespace BeeeOn {

class SensorData;

/**
 * Interface to report events occuring in Distributor
 */
class DistributorListener
{
public:
	DistributorListener();
	virtual ~DistributorListener();

	/**
	 * @brief notifyListener
	 * @param data
	 * This method is called before the Distributor starts
	 * exporting the given data
	 */
	virtual void onExport(const SensorData &data) = 0;
};

}
