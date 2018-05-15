#include <Poco/Exception.h>

#include "commands/ServerDeviceListCommand.h"
#include "commands/ServerDeviceListResult.h"
#include "commands/ServerLastValueCommand.h"
#include "commands/ServerLastValueResult.h"
#include "core/DeviceManager.h"
#include "core/MemoryDeviceCache.h"
#include "core/PrefixCommand.h"
#include "util/ClassInfo.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const Timespan DeviceManager::DEFAULT_REQUEST_TIMEOUT(5 * Timespan::SECONDS);

DeviceManager::DeviceManager(const DevicePrefix &prefix,
		const initializer_list<type_index> &acceptable):
	m_stop(false),
	m_prefix(prefix),
	m_deviceCache(new MemoryDeviceCache),
	m_acceptable(acceptable)
{
}

DeviceManager::~DeviceManager()
{
}

DevicePrefix DeviceManager::prefix() const
{
	return m_prefix;
}

void DeviceManager::stop()
{
	m_stop = true;
}

void DeviceManager::setDeviceCache(DeviceCache::Ptr cache)
{
	m_deviceCache = cache;
}

DeviceCache::Ptr DeviceManager::deviceCache() const
{
	return m_deviceCache;
}

void DeviceManager::setDistributor(Poco::SharedPtr<Distributor> distributor)
{
	m_distributor = distributor;
}

bool DeviceManager::accept(const Command::Ptr cmd)
{
	if (m_acceptable.find(typeid(*cmd)) == m_acceptable.end())
		return false;

	if (cmd->is<PrefixCommand>()) {
		const auto pcmd = cmd.cast<PrefixCommand>();
		return pcmd->prefix() == m_prefix;
	}

	return true;
}

void DeviceManager::handle(Command::Ptr cmd, Answer::Ptr answer)
{
	Result::Ptr result = cmd->deriveResult(answer);

	try {
		handleGeneric(cmd, result);
		result->setStatus(Result::Status::SUCCESS);
	}
	BEEEON_CATCH_CHAIN_ACTION(logger(),
		result->setStatus(Result::Status::FAILED)
	)
}

void DeviceManager::handleGeneric(const Command::Ptr cmd, Result::Ptr)
{
	if (cmd->is<DeviceAcceptCommand>())
		handleAccept(cmd.cast<DeviceAcceptCommand>());
	else
		throw NotImplementedException(cmd->toString());
}

void DeviceManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	const auto &id = cmd->deviceID();

	if (id.prefix() != m_prefix)
		throw AssertionViolationException("incompatible prefix: " + id.prefix().toString());

	m_deviceCache->markPaired(id);
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
		if (answer->at(i)->status() != Result::Status::SUCCESS) {
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

double DeviceManager::lastValue(const DeviceID &deviceID,
	const ModuleID &moduleID, const Timespan &waitTime)
{
	Answer::Ptr answer = new Answer(answerQueue());

	ServerLastValueCommand::Ptr cmd =
		new ServerLastValueCommand(deviceID, moduleID);

	dispatch(cmd, answer);
	answer->waitNotPending(waitTime);

	int failedResults = 0;
	ServerLastValueResult::Ptr result;

	for (unsigned long i = 0; i < answer->resultsCount(); i++) {
		if (answer->at(i)->status() != Result::Status::SUCCESS) {
			failedResults++;
			continue;
		}

		if (answer->at(i).cast<ServerLastValueResult>().isNull()) {
			logger().critical(
				"unexpected result " + ClassInfo::forPointer(answer->at(i).get()).name()
				+ " for device: " + deviceID.toString(), __FILE__, __LINE__);
			failedResults++;
			continue;
		}

		if (result.isNull())
			result = answer->at(i).cast<ServerLastValueResult>();
	}

	answerQueue().remove(answer);

	if (result.isNull()) {
		throw Exception(
			"failed to obtain last value for device: "
			+ deviceID.toString());
	}

	if (failedResults > 0) {
		logger().warning(
			to_string(failedResults)
			+ "/" + to_string(answer->resultsCount())
			+ " results has failed when waiting for last value for "
			+ deviceID.toString(),
			__FILE__, __LINE__
		);
	}

	return result->value();
}
