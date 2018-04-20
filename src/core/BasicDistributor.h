#pragma once

#include <Poco/Mutex.h>

#include "core/AbstractDistributor.h"

namespace BeeeOn {

class SensorData;

class BasicDistributor : public AbstractDistributor {
public:
	/*
	 * Export data to all registered exporters.
	 */
	void exportData(const SensorData &sensorData) override;
private:
	Poco::FastMutex m_exportMutex;
};

}
