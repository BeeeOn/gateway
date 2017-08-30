#include <Poco/Logger.h>
#include <Poco/SharedPtr.h>

#include "core/AbstractDistributor.h"
#include "core/DistributorListener.h"
#include "core/Exporter.h"
#include "model/SensorData.h"

using namespace BeeeOn;

void AbstractDistributor::registerExporter(Poco::SharedPtr<Exporter> exporter)
{
	poco_debug(logger(), "registering new exporter");
	m_exporters.push_back(exporter);
}

void AbstractDistributor::notifyListeners(const SensorData &data)
{
	std::vector<DistributorListener::Ptr> listeners = m_listeners;

	if (!m_executor.isNull()) {
		m_executor->invoke([listeners, data]() {
			for (auto listener : listeners)
				listener->onExport(data);
		});
	}
}

void AbstractDistributor::registerListener(DistributorListener::Ptr listener)
{
	m_listeners.push_back(listener);
}

void AbstractDistributor::setExecutor(Poco::SharedPtr<AsyncExecutor> executor)
{
	m_executor = executor;
}
