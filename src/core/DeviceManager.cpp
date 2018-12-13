#include <Poco/Clock.h>
#include <Poco/Exception.h>

#include "commands/DeviceUnpairResult.h"
#include "commands/ServerDeviceListCommand.h"
#include "commands/ServerDeviceListResult.h"
#include "commands/ServerLastValueCommand.h"
#include "commands/ServerLastValueResult.h"
#include "core/DeviceManager.h"
#include "core/MemoryDeviceCache.h"
#include "core/PrefixCommand.h"
#include "model/SensorData.h"
#include "util/ClassInfo.h"
#include "util/MultiException.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

DeviceManager::DeviceManager(const DevicePrefix &prefix,
		const initializer_list<type_index> &acceptable):
	m_prefix(prefix),
	m_deviceCache(new MemoryDeviceCache),
	m_acceptable(acceptable),
	m_remoteStatusDelivered(false)
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
	m_stopControl.requestStop();
	m_cancellable.cancel();
}

void DeviceManager::setDeviceCache(DeviceCache::Ptr cache)
{
	m_deviceCache = cache;
}

DeviceCache::Ptr DeviceManager::deviceCache() const
{
	return m_deviceCache;
}

CancellableSet &DeviceManager::cancellable()
{
	return m_cancellable;
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

void DeviceManager::handleGeneric(const Command::Ptr cmd, Result::Ptr result)
{
	if (cmd->is<DeviceAcceptCommand>())
		handleAccept(cmd.cast<DeviceAcceptCommand>());
	else if (cmd->is<GatewayListenCommand>())
		handleListen(cmd.cast<GatewayListenCommand>());
	else if (cmd->is<DeviceSearchCommand>())
		handleSearch(cmd.cast<DeviceSearchCommand>());
	else if (cmd->is<DeviceUnpairCommand>()) {
		DeviceUnpairResult::Ptr unpair = result.cast<DeviceUnpairResult>();
		poco_assert(!unpair.isNull());

		const auto &result = handleUnpair(cmd.cast<DeviceUnpairCommand>());
		unpair->setUnpaired(result);
	}
	else if (cmd->is<DeviceSetValueCommand>()) {
		handleSetValue(cmd.cast<DeviceSetValueCommand>());
	}
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

AsyncWork<>::Ptr DeviceManager::startDiscovery(const Timespan &)
{
	throw NotImplementedException("generic discovery is not supported");
}

void DeviceManager::handleListen(const GatewayListenCommand::Ptr cmd)
{
	const Clock started;
	const Timespan &duration = cmd->duration();

	if (duration < 1 * Timespan::SECONDS) {
		throw InvalidArgumentException(
			"listen duration is too short: "
			+ to_string(duration.totalMicroseconds()) + " us");
	}

	ScopedLock<FastMutex> guard(m_listenLock, duration.totalMilliseconds());

	const Timespan &timeout = checkDelayedOperation("discovery", started, duration);

	logger().information("starting discovery (" + to_string(timeout.totalSeconds()) + " s)", __FILE__, __LINE__);

	auto discovery = startDiscovery(timeout);
	manageUntilFinished("discovery", discovery, timeout);
}

AsyncWork<>::Ptr DeviceManager::startSearch(
		const Timespan &,
		const Poco::Net::IPAddress &)
{
	throw NotImplementedException("generic search-by-ip-address is not supported");
}

AsyncWork<>::Ptr DeviceManager::startSearch(
		const Timespan &,
		const MACAddress &)
{
	throw NotImplementedException("generic search-by-mac-address is not supported");
}

AsyncWork<>::Ptr DeviceManager::startSearch(
		const Timespan &,
		const uint64_t)
{
	throw NotImplementedException("generic search-by-serial-number is not supported");
}

void DeviceManager::handleSearch(const DeviceSearchCommand::Ptr cmd)
{
	const Clock started;
	const Timespan &duration = cmd->duration();

	if (duration < 1 * Timespan::SECONDS) {
		throw InvalidArgumentException(
			"search duration is too short: "
			+ to_string(duration.totalMicroseconds()) + " us");
	}

	ScopedLock<FastMutex> guard(m_listenLock, duration.totalMilliseconds());

	const Timespan &timeout = checkDelayedOperation("search", started, duration);

	logger().information("starting search (" + to_string(timeout.totalSeconds()) + " s)", __FILE__, __LINE__);

	AsyncWork<>::Ptr search;

	if (cmd->hasIPAddress())
		search = startSearch(timeout, cmd->ipAddress());
	else if (cmd->hasMACAddress())
		search = startSearch(timeout, cmd->macAddress());
	else if (cmd->hasSerialNumber())
		search = startSearch(timeout, cmd->serialNumber());
	else
		poco_assert_msg(0, "missing search criteria");

	manageUntilFinished("search", search, timeout);
}

AsyncWork<set<DeviceID>>::Ptr DeviceManager::startUnpair(
		const DeviceID &,
		const Timespan &)
{
	throw NotImplementedException("generic unpair is not supported");
}

set<DeviceID> DeviceManager::handleUnpair(const DeviceUnpairCommand::Ptr cmd)
{
	const Clock started;
	const Timespan &duration = cmd->timeout();

	ScopedLock<FastMutex> guard(m_unpairLock, duration.totalMilliseconds());

	const Timespan &timeout = checkDelayedOperation("unpair", started, duration);

	logger().information("starting unpair of " + cmd->deviceID().toString(), __FILE__, __LINE__);

	auto unpair = startUnpair(cmd->deviceID(), timeout);
	manageUntilFinished("unpair", unpair, timeout);

	if (unpair->result().isNull())
		return {};

	const set<DeviceID> result = unpair->result();

	if (result.find(cmd->deviceID()) != result.end() && result.size() == 1) {
		logger().information("unpair was successful", __FILE__, __LINE__);
	}
	else if (result.empty()) {
		logger().warning("unpair seems to be unsuccessful", __FILE__, __LINE__);
	}
	else {
		string ids;

		for (const auto &id : result) {
			if (!ids.empty())
				ids += ", ";

			ids += id.toString();
		}

		logger().notice("unpaired devices: " + ids, __FILE__, __LINE__);
	}

	return result;
}

AsyncWork<double>::Ptr DeviceManager::startSetValueByMode(
		const DeviceID &id,
		const ModuleID &module,
		const double value,
		const OpMode &mode,
		const Timespan &timeout)
{
	logger().information(
		"starting set-value " + to_string(value)
		+ " of " + id.toString() + " [" + module.toString() + "]"
		+ " in mode " + mode.toString(),
		__FILE__, __LINE__);

	switch (mode.raw()) {
	case OpMode::TRY_ONCE:
		return startSetValue(id, module, value, timeout);

	case OpMode::TRY_HARDER:
		return startSetValueTryHarder(id, module, value, timeout);

	case OpMode::REPEAT_ON_FAIL:
		return startSetValueRepeatOnFail(id, module, value, timeout);
	}

	throw IllegalStateException("unimplemented operation mode " + mode.toString());
}

AsyncWork<double>::Ptr DeviceManager::startSetValue(
		const DeviceID &,
		const ModuleID &,
		const double,
		const Timespan &)
{
	throw NotImplementedException("generic set-value is not supported");
}

AsyncWork<double>::Ptr DeviceManager::startSetValueTryHarder(
		const DeviceID &id,
		const ModuleID &module,
		const double value,
		const Timespan &timeout)
{
	return startSetValue(id, module, value, timeout);
}

AsyncWork<double>::Ptr DeviceManager::startSetValueRepeatOnFail(
		const DeviceID &id,
		const ModuleID &module,
		const double value,
		const Timespan &timeout)
{
	const Clock started;
	MultiException me;

	while (!started.isElapsed(timeout.totalMicroseconds())) {
		if (m_stopControl.shouldStop()) {
			if (me.count() > 0)
				break;

			throw IllegalStateException(
				"device manager stop has been requested during"
				" set-value in mode repeat_on_fail");
		}

		try {
			return startSetValue(id, module, value, timeout);
		}
		catch (const IOException &e) {
			Loggable::logException(
				logger(), Message::PRIO_WARNING, e, __FILE__, __LINE__);
			me.caught(e);
		}
	}

	poco_assert(me.count() > 0);

	me.rethrow();
	throw IllegalStateException("never happens");
}

void DeviceManager::handleSetValue(const DeviceSetValueCommand::Ptr cmd)
{
	const Clock started;
	const Timespan &duration = cmd->timeout();

	ScopedLock<FastMutex> guard(m_setValueLock, duration.totalMilliseconds());

	const Timespan &timeout = checkDelayedOperation("set-value", started, duration);

	auto operation = startSetValueByMode(
		cmd->deviceID(),
		cmd->moduleID(),
		cmd->value(),
		cmd->mode(),
		timeout);
	manageUntilFinished("set-value", operation, timeout);

	if (operation->result().isNull())
		throw IllegalStateException("result of set-value was not provided");

	const auto value = operation->result().value();

	try {
		if (logger().debug()) {
			logger().debug(
				"shipping value " + to_string(value)
				+ " that has just been set",
				__FILE__, __LINE__);
		}

		ship({
			cmd->deviceID(),
			Timestamp{},
			{{cmd->moduleID(), value}}
		});
	}
	BEEEON_CATCH_CHAIN(logger())
}

void DeviceManager::ship(const SensorData &sensorData)
{
	m_distributor->exportData(sensorData);
}

Timespan DeviceManager::checkDelayedOperation(
		const string &opname,
		const Clock &started,
		const Timespan &duration) const
{
	if (started.elapsed() > 1 * Timespan::SECONDS) {
		logger().warning(opname + " has been significantly delayed: "
			+ to_string(started.elapsed()) + " us",
			__FILE__, __LINE__);
	}

	if (m_stopControl.shouldStop())
		throw IllegalStateException(opname + " skipped due to shutdown request");

	Timespan timeout = duration - started.elapsed();
	if (timeout < 1 * Timespan::SECONDS)
		timeout = 1 * Timespan::SECONDS;

	return timeout;
}

bool DeviceManager::manageUntilFinished(
		const string &opname,
		AnyAsyncWork::Ptr work,
		const Timespan &timeout)
{
	cancellable().manage(work);

	if (!work->tryJoin(timeout)) {
		if (cancellable().unmanage(work)) {
			logger().information("cancelling " + opname, __FILE__, __LINE__);
			work->cancel();
		}

		logger().information(opname + " has been cancelled", __FILE__, __LINE__);
		return false;
	}
	else {
		cancellable().unmanage(work);
		return true;
	}
}

void DeviceManager::handleRemoteStatus(
		const DevicePrefix &prefix,
		const set<DeviceID> &paired,
		const DeviceStatusHandler::DeviceValues &)
{
	if (this->prefix() != prefix) {
		logger().warning(
			"unexpected prefix " + prefix.toString()
			+ " wanted " + this->prefix().toString(),
			__FILE__, __LINE__);
		return;
	}

	deviceCache()->markPaired(prefix, paired);
	m_remoteStatusDelivered = true;
	m_stopControl.requestWakeup();
}

set<DeviceID> DeviceManager::waitRemoteStatus(const Poco::Timespan &timeout)
{
	while (!m_stopControl.shouldStop() && !m_remoteStatusDelivered)
		m_stopControl.waitStoppable(timeout);

	if (m_remoteStatusDelivered)
		return deviceCache()->paired(prefix());

	return {};
}
