#pragma once

namespace BeeeOn {

class SensorData;

/*
 * Distributor distributes data to registered Exporters.
 */
class Distributor {
public:
	/*
	 * Export data to all registered exporters.
	 */
	virtual void exportData(const SensorData &sensorData) = 0;
};

}
