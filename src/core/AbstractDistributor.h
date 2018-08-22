#pragma once

#include <vector>

#include <Poco/SharedPtr.h>

#include "core/Distributor.h"
#include "core/DistributorListener.h"
#include "util/EventSource.h"
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

	void registerListener(DistributorListener::Ptr listener);

	/*
	 * Set executor instance for asynchronous data transfer to
	 * listeners.
	 */
	void setExecutor(AsyncExecutor::Ptr executor);

protected:
	/*
	 * Notify registered listeners by calling onExport() method.
	 * This is supposed to be called at the beginning of Distributor::export().
	 */
	void notifyListeners(const SensorData &data);

	std::vector<Poco::SharedPtr<Exporter>> m_exporters;
	EventSource<DistributorListener> m_eventSource;
};

}
