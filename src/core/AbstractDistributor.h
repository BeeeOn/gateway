#pragma once

#include <vector>

#include <Poco/SharedPtr.h>

#include "core/Distributor.h"
#include "util/Loggable.h"

namespace BeeeOn {

class Exporter;
class SensorData;

class AbstractDistributor : public Distributor, public Loggable {
public:
	/*
	 * Register exporter. Received messages are resent to
	 * all registered exporters.
	*/
	virtual void registerExporter(Poco::SharedPtr<Exporter> exporter);
	/*
	 * Export data to all registered exporters.
	 */
	virtual void exportData(const SensorData &sensorData) = 0;

protected:
	std::vector<Poco::SharedPtr<Exporter>> m_exporters;
};

}
