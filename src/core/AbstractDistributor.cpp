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
	m_eventSource.fireEvent(data, &DistributorListener::onExport);
}

void AbstractDistributor::registerListener(DistributorListener::Ptr listener)
{
	m_eventSource.addListener(listener);
}

void AbstractDistributor::setExecutor(Poco::SharedPtr<AsyncExecutor> executor)
{
	m_eventSource.setAsyncExecutor(executor);
}
