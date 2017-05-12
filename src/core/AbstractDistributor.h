#pragma once

#include <vector>

#include <Poco/SharedPtr.h>

#include "core/Distributor.h"
#include "core/DistributorListener.h"
#include "util/AsyncExecutor.h"
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

	void registerListener(Poco::SharedPtr<DistributorListener> listener);

	/*
	 * Set executor instance for asynchronous data transfer to
	 * listeners.
	 */
	void setExecutor(Poco::SharedPtr<AsyncExecutor> executor);

protected:
	/*
	 * Notify registered listeners by calling onExport() method.
	 * This is supposed to be called at the beginning of Distributor::export().
	 */
	void notifyListeners(const SensorData &data);

	std::vector<Poco::SharedPtr<Exporter>> m_exporters;
	std::vector<Poco::SharedPtr<DistributorListener>> m_listeners;
	Poco::SharedPtr<AsyncExecutor> m_executor;
};

}
