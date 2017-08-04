#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>

#include <exception>
#include <string>

#include "di/Injectable.h"
#include "core/BasicDistributor.h"
#include "core/Exporter.h"
#include "model/SensorData.h"

BEEEON_OBJECT_BEGIN(BeeeOn, BasicDistributor)
BEEEON_OBJECT_CASTABLE(Distributor)
BEEEON_OBJECT_REF("exporter", &BasicDistributor::registerExporter)
BEEEON_OBJECT_REF("listener", &BasicDistributor::registerListener)
BEEEON_OBJECT_REF("executor", &BasicDistributor::setExecutor)
BEEEON_OBJECT_END(BeeeOn, BasicDistributor)

using namespace BeeeOn;

void BasicDistributor::exportData(const SensorData &sensorData)
{
	Poco::FastMutex::ScopedLock lock(m_exportMutex);

	notifyListeners(sensorData);

	for (Poco::SharedPtr<Exporter> exporter : m_exporters) {
		try {
			exporter->ship(sensorData);
			poco_debug(logger(), "Data shipped successfully");

		} catch (Poco::Exception &ex) {
			poco_error(logger(), "Data failed to ship: " + ex.displayText());

		} catch (std::exception &ex) {
			poco_critical(logger(), "Data failed to ship: " + std::string(ex.what()));

		} catch (...) {
			poco_critical(logger(), "Unknown error occurred when shipping data");
		}
	}
}
