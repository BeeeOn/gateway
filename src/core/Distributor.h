#pragma once

#include <Poco/SharedPtr.h>

namespace BeeeOn {

class SensorData;

/*
 * Distributor distributes data to registered Exporters.
 */
class Distributor {
public:
	typedef Poco::SharedPtr<Distributor> Ptr;

	virtual ~Distributor();

	/*
	 * Export data to all registered exporters.
	 */
	virtual void exportData(const SensorData &sensorData) = 0;
};

}
