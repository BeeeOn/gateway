#include "commands/ServerDeviceListCommand.h"
#include "commands/ServerDeviceListResult.h"
#include "core/DeviceManager.h"
#include "util/ClassInfo.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const Timespan DeviceManager::DEFAULT_REQUEST_TIMEOUT(5 * Timespan::SECONDS);

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

set<DeviceID> DeviceManager::deviceList(const Poco::Timespan &timeout)
{
	Answer::Ptr answer = new Answer(answerQueue());

	requestDeviceList(answer);
	return responseDeviceList(timeout, answer);
}

void DeviceManager::requestDeviceList(Answer::Ptr answer)
{
	ServerDeviceListCommand::Ptr cmd =
		new ServerDeviceListCommand(m_prefix);

	dispatch(cmd, answer);
}

set<DeviceID> DeviceManager::responseDeviceList(
	const Timespan &waitTime, Answer::Ptr answer)
{
	set<DeviceID> devices;
	unsigned long failedResults = 0;

	answer->waitNotPending(waitTime);
	for (unsigned long i = 0; i < answer->resultsCount(); i++) {
		if (answer->at(i)->status() != Result::SUCCESS) {
			failedResults++;
			continue;
		}

		ServerDeviceListResult::Ptr res =
			answer->at(i).cast<ServerDeviceListResult>();

		if (res.isNull()) {
			logger().critical(
				"unexpected result " + ClassInfo::forPointer(answer->at(i).get()).name()
				+ " for prefix: " + m_prefix, __FILE__, __LINE__);
			failedResults++;
			continue;
		}

		for (auto &id : res->deviceList()) {
			auto ret = devices.insert(id);

			if (!ret.second) {
				logger().warning(
					"duplicate device: " + id.toString()
					+ " for " + m_prefix.toString(),
					__FILE__, __LINE__);
			}
		}
	}

	answerQueue().remove(answer);

	if (failedResults == answer->resultsCount()) {
		throw Exception(
			to_string(failedResults)
			+ "/" + to_string(answer->resultsCount())
			+ " results has failed when waiting device list for "
			+ m_prefix.toString()
		);
	}

	if (failedResults > 0) {
		logger().warning(
			to_string(failedResults)
			+ "/" + to_string(answer->resultsCount())
			+ " results has failed while receiving device list",
			__FILE__, __LINE__
		);
	}

	return devices;
}
