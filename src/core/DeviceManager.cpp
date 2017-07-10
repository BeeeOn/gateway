#include "core/DeviceManager.h"

using namespace BeeeOn;
using namespace Poco;

DeviceManager::DeviceManager(const std::string &name,
		const DevicePrefix &prefix):
	CommandHandler(name),
	m_stop(false),
	m_prefix(prefix)
{
}

DeviceManager::~DeviceManager()
{
}

void DeviceManager::stop()
{
	m_stop = true;
}

void DeviceManager::setDistributor(Poco::SharedPtr<Distributor> distributor)
{
	m_distributor = distributor;
}

void DeviceManager::ship(const SensorData &sensorData)
{
	m_distributor->exportData(sensorData);
}
