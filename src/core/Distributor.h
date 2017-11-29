#pragma once

namespace BeeeOn {

class SensorData;

/*
 * Distributor distributes data to registered Exporters.
 */
class Distributor {
public:
	virtual ~Distributor();

	/*
	 * Export data to all registered exporters.
	 */
	virtual void exportData(const SensorData &sensorData) = 0;
};

}
