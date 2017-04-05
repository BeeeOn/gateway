#include <Poco/Logger.h>
#include <Poco/SharedPtr.h>

#include "core/AbstractDistributor.h"
#include "core/Exporter.h"

using namespace BeeeOn;

void AbstractDistributor::registerExporter(Poco::SharedPtr<Exporter> exporter)
{
	poco_debug(logger(), "registering new exporter");
	m_exporters.push_back(exporter);
}
