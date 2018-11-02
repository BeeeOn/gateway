#include <vector>

#include <Poco/Clock.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "commands/GatewayListenCommand.h"
#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "model/SensorData.h"
#include "util/BlockingAsyncWork.h"
#include "util/ClassInfo.h"
#include "zwave/ZWaveDeviceManager.h"

BEEEON_OBJECT_BEGIN(BeeeOn, ZWaveDeviceManager)
BEEEON_OBJECT_CASTABLE(DeviceManager)
BEEEON_OBJECT_CASTABLE(DeviceStatusHandler)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_PROPERTY("deviceCache", &ZWaveDeviceManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("network", &ZWaveDeviceManager::setNetwork)
BEEEON_OBJECT_PROPERTY("registry", &ZWaveDeviceManager::setRegistry)
BEEEON_OBJECT_PROPERTY("dispatchDuration", &ZWaveDeviceManager::setDispatchDuration)
BEEEON_OBJECT_PROPERTY("pollTimeout", &ZWaveDeviceManager::setPollTimeout)
BEEEON_OBJECT_PROPERTY("distributor", &ZWaveDeviceManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &ZWaveDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_END(BeeeOn, ZWaveDeviceManager)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

ZWaveDeviceManager::Device::Device(const ZWaveNode &node):
	m_node(node),
	m_refresh(0)
{
}

DeviceID ZWaveDeviceManager::Device::id() const
{
	poco_assert_msg(!m_mapper.isNull(), "mapper not resolved");
	return m_mapper->buildID();
}

string ZWaveDeviceManager::Device::product() const
{
	poco_assert_msg(!m_mapper.isNull(), "mapper not resolved");
	return m_mapper->product();
}

string ZWaveDeviceManager::Device::vendor() const
{
	return m_node.vendor();
}

void ZWaveDeviceManager::Device::updateNode(const ZWaveNode &node)
{
	poco_assert_msg(node.id() == m_node.id(), "updating non-matching node");

	if (m_node.queried() && !node.queried())
		return;

	m_node = node;
}

const ZWaveNode &ZWaveDeviceManager::Device::node() const
{
	return m_node;
}

bool ZWaveDeviceManager::Device::resolveMapper(
		ZWaveMapperRegistry::Ptr registry)
{
	if (m_mapper.isNull())
		m_mapper = registry->resolve(m_node);

	return !m_mapper.isNull();
}

ZWaveMapperRegistry::Mapper::Ptr ZWaveDeviceManager::Device::mapper() const
{
	return m_mapper;
}

void ZWaveDeviceManager::Device::setRefresh(const Timespan &refresh)
{
	m_refresh = refresh;
}

Timespan ZWaveDeviceManager::Device::refresh() const
{
	return m_refresh;
}

list<ModuleType> ZWaveDeviceManager::Device::types() const
{
	poco_assert_msg(!m_mapper.isNull(), "mapper not resolved");
	return m_mapper->types();
}

SensorValue ZWaveDeviceManager::Device::convert(const ZWaveNode::Value &value) const
{
	poco_assert_msg(!m_mapper.isNull(), "mapper not resolved");
	return m_mapper->convert(value);
}

string ZWaveDeviceManager::Device::toString() const
{
	return id().toString() + " " + m_node.toString();
}

ZWaveDeviceManager::ZWaveDeviceManager():
	DeviceManager(DevicePrefix::PREFIX_ZWAVE, {
		typeid(GatewayListenCommand),
		typeid(DeviceAcceptCommand),
		typeid(DeviceUnpairCommand),
		typeid(DeviceSetValueCommand),
	}),
	m_dispatchDuration(60 * Timespan::SECONDS),
	m_pollTimeout(30 * Timespan::SECONDS)
{
}

void ZWaveDeviceManager::setNetwork(ZWaveNetwork::Ptr network)
{
	m_network = network;
}

void ZWaveDeviceManager::setRegistry(ZWaveMapperRegistry::Ptr registry)
{
	m_registry = registry;
}

void ZWaveDeviceManager::setDispatchDuration(const Timespan &duration)
{
	if (duration < 1 * Timespan::MILLISECONDS)
		throw InvalidArgumentException("dispatchDuration must be at least 1 ms");

	m_dispatchDuration = duration;
}

void ZWaveDeviceManager::setPollTimeout(const Timespan &timeout)
{
	if (timeout < 1 * Timespan::MILLISECONDS)
		throw InvalidArgumentException("pollTimeout must be at least 1 ms");

	m_pollTimeout = timeout;
}

void ZWaveDeviceManager::run()
{
	logger().information("Z-Wave device manager is starting");

	Clock lastInclusion = 0;
	StopControl::Run run(m_stopControl);

	while (run) {
		const auto event = m_network->pollEvent(m_pollTimeout);

		if (logger().trace())
			logger().trace(event.toString(), __FILE__, __LINE__);

		switch (event.type()) {
		case ZWaveNetwork::PollEvent::EVENT_NONE:
			break;

		case ZWaveNetwork::PollEvent::EVENT_VALUE:
			processValue(event.value());
			break;

		case ZWaveNetwork::PollEvent::EVENT_NEW_NODE:
			newNode(event.node(), !lastInclusion.isElapsed(
					m_dispatchDuration.totalMicroseconds()));
			break;

		case ZWaveNetwork::PollEvent::EVENT_UPDATE_NODE:
			updateNode(event.node(), !lastInclusion.isElapsed(
					m_dispatchDuration.totalMicroseconds()));
			break;

		case ZWaveNetwork::PollEvent::EVENT_REMOVE_NODE:
			removeNode(event.node());
			break;

		case ZWaveNetwork::PollEvent::EVENT_INCLUSION_START:
			lastInclusion.update();
			break;

		case ZWaveNetwork::PollEvent::EVENT_INCLUSION_DONE:
			lastInclusion.update();
			m_inclusionWork->cancel();
			break;

		case ZWaveNetwork::PollEvent::EVENT_REMOVE_NODE_DONE:
			m_removeNodeWork->cancel();
			break;

		default:
			break;
		}

		if (logger().trace())
			logger().trace("event handled", __FILE__, __LINE__);
	}

	logger().information("Z-Wave device manager has stopped");
}

void ZWaveDeviceManager::stop()
{
	logger().information("stopping Z-Wave device manager");

	DeviceManager::stop();
	m_network->interrupt();
}

void ZWaveDeviceManager::processValue(const ZWaveNode::Value &value)
{
	FastMutex::ScopedLock guard(m_lock);

	auto it = m_zwaveNodes.find(value.node());
	if (it == m_zwaveNodes.end()) {
		if (logger().trace()) {
			logger().trace(
				"ignoring value " + value.value()
				+ " for non-registered device "
				+ value.node().toString(),
				__FILE__, __LINE__);
		}

		return;
	}

	Device &device = it->second->second;
	poco_assert(!device.mapper().isNull());

	const auto &cc = value.commandClass();

	if (cc.id() == ZWaveNode::CommandClass::WAKE_UP && cc.index() == 0) {
		const auto time = value.asTime();

		if (logger().debug()) {
			logger().debug(
				"update refresh time of "
				+ device.id().toString()
				+ " to " + DateTimeFormatter::format(time),
				__FILE__, __LINE__);
		}

		device.setRefresh(time);
		return;
	}

	if (!deviceCache()->paired(device.id())) {
		if (logger().trace()) {
			logger().trace(
				"value for non-paired device "
				+ device.id().toString() + " is dropped",
				__FILE__, __LINE__);
		}

		return;
	}

	try {
		const Timestamp now;
		ship({device.id(), now, {device.convert(value)}});
	}
	BEEEON_CATCH_CHAIN(logger())
}

void ZWaveDeviceManager::newNode(const ZWaveNode &node, bool dispatch)
{
	FastMutex::ScopedLock guard(m_lock);
	newNodeUnlocked(node, dispatch);
}

void ZWaveDeviceManager::newNodeUnlocked(const ZWaveNode &node, bool dispatch)
{
	if (logger().debug()) {
		logger().debug(
			"inspecting a new Z-Wave node " + node.toString()
			+ (dispatch? " (dispatching)" : " (not dispatching)"),
			__FILE__, __LINE__);
	}

	auto it = m_zwaveNodes.find(node.id());
	if (it != m_zwaveNodes.end()) {
		logger().warning(
			"node " + node.toString() + " already exists, ignoring...",
			__FILE__, __LINE__);
		return;
	}

	Device device(node);
	if (!device.resolveMapper(m_registry)) {
		logger().warning(
			"unable to resolve mapper for " + node.toString(),
			__FILE__, __LINE__);
		return;
	}

	if (logger().debug()) {
		logger().debug(
			"device " + device.id().toString() + " "
			+ device.product() + " resolved to mapper "
			+ ClassInfo::forPointer(device.mapper().get()).name(),
			__FILE__, __LINE__);
	}

	registerDevice(device);

	if (!deviceCache()->paired(device.id()))
		dispatchDevice(device, dispatch);
}

void ZWaveDeviceManager::updateNode(const ZWaveNode &node, bool dispatch)
{
	FastMutex::ScopedLock guard(m_lock);

	auto it = m_zwaveNodes.find(node.id());
	if (it == m_zwaveNodes.end()) {
		newNodeUnlocked(node, dispatch);
		return;
	}

	if (logger().debug()) {
		logger().debug(
			"updating Z-Wave node " + node.toString()
			+ (dispatch? " (dispatching)" : " (not dispatching)"),
			__FILE__, __LINE__);
	}

	Device &device = it->second->second;
	device.updateNode(node);

	if (!deviceCache()->paired(device.id()))
		dispatchDevice(device, dispatch);
}

void ZWaveDeviceManager::removeNode(const ZWaveNode &node)
{
	logger().information(
		"removing Z-Wave node " + node.toString(),
		__FILE__, __LINE__);

	FastMutex::ScopedLock guard(m_lock);

	auto it = m_zwaveNodes.find(node.id());
	if (it == m_zwaveNodes.end()) {
		if (logger().debug()) {
			logger().debug(
				"no such Z-Wave node " + node.toString()
				+ " to be unregistered",
				__FILE__, __LINE__);
		}

		return;
	}

	const auto device = unregisterDevice(it);

	deviceCache()->markUnpaired(device.id());
	m_recentlyUnpaired.emplace(device.id());
}

void ZWaveDeviceManager::registerDevice(const Device &device)
{
	poco_assert(!device.mapper().isNull());

	if (logger().debug()) {
		logger().debug(
			"registering device " + device.id().toString(),
			__FILE__, __LINE__);
	}

	auto result = m_devices.emplace(device.id(), device);
	m_zwaveNodes.emplace(device.node().id(), result.first);
}

ZWaveDeviceManager::Device ZWaveDeviceManager::unregisterDevice(
		ZWaveNodeMap::iterator it)
{
	const auto devit = it->second;
	const Device copy = devit->second;

	if (logger().debug()) {
		logger().debug(
			"unregistering device " + copy.id().toString(),
			__FILE__, __LINE__);
	}

	m_zwaveNodes.erase(it);
	m_devices.erase(devit);

	return copy;
}

void ZWaveDeviceManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	FastMutex::ScopedLock guard(m_lock);

	auto it = m_devices.find(cmd->deviceID());
	if (it == m_devices.end()) {
		throw NotFoundException(
			"no such device "
			+ cmd->deviceID().toString()
			+ " to accept");
	}

	DeviceManager::handleAccept(cmd);

	logger().information(
		"device " + cmd->deviceID().toString() + " has been paired",
		__FILE__, __LINE__);
}

AsyncWork<>::Ptr ZWaveDeviceManager::startDiscovery(const Timespan &duration)
{
	m_network->startInclusion();

	m_inclusionWork = new DelayedAsyncWork<>(
		[this](DelayedAsyncWork<> &) {stopInclusion();},
		duration);

	dispatchUnpaired();

	return m_inclusionWork;
}

void ZWaveDeviceManager::dispatchUnpaired()
{
	FastMutex::ScopedLock guard(m_lock);

	if (logger().debug()) {
		logger().debug(
			"dispatching " + to_string(m_devices.size())
			+ " non-paired devices",
			__FILE__, __LINE__);
	}

	for (const auto &device : m_devices)
		dispatchDevice(device.second);
}

void ZWaveDeviceManager::dispatchDevice(const Device &device, bool enabled)
{
	poco_assert(!device.mapper().isNull());

	if (deviceCache()->paired(device.id())) {
		logger().debug(
			"device " + device.toString() + " is already paired",
			__FILE__, __LINE__);
		return;
	}

	if (device.node().controller()) {
		logger().debug(
			"device " + device.toString() + " is a controller",
			__FILE__, __LINE__);
		return;
	}

	if (!enabled) {
		logger().warning(
			"avoid dispatching of device " + device.toString()
			+ " out of listening mode",
			__FILE__, __LINE__);
		return;
	}

	logger().information("dispatching new device " + device.toString(),
			__FILE__, __LINE__);

	const auto description = DeviceDescription::Builder()
		.id(device.id())
		.type(device.vendor(), device.product())
		.modules(device.types())
		.refreshTime(device.refresh())
		.build();

	dispatch(new NewDeviceCommand(description));
}

set<DeviceID> ZWaveDeviceManager::recentlyUnpaired()
{
	FastMutex::ScopedLock guard(m_lock);

	const auto copy = m_recentlyUnpaired;
	m_recentlyUnpaired.clear();

	return copy;
}

AsyncWork<set<DeviceID>>::Ptr ZWaveDeviceManager::startUnpair(
		const DeviceID &,
		const Timespan &timeout)
{
	FastMutex::ScopedLock guard(m_lock);
	m_recentlyUnpaired.clear();

	m_network->startRemoveNode();

	DelayedAsyncWork<set<DeviceID>>::Ptr work;

	auto success = [this](DelayedAsyncWork<set<DeviceID>> &self) mutable {
		stopRemoveNode();
		self.setResult(recentlyUnpaired());
	};
	auto cancel = [this](DelayedAsyncWork<set<DeviceID>> &self) mutable {
		self.setResult(recentlyUnpaired());
	};

	m_removeNodeWork = new DelayedAsyncWork<set<DeviceID>>(success, cancel, timeout);
	return m_removeNodeWork;
}

void ZWaveDeviceManager::stopInclusion()
{
	logger().information(
		"stopping the Z-Wave inclusion process",
		__FILE__, __LINE__);

	try {
		m_network->cancelInclusion();
	}
	catch (const IllegalStateException &e) {
		logger().warning(e.displayText(), __FILE__, __LINE__);
	}
	BEEEON_CATCH_CHAIN(logger())
}

void ZWaveDeviceManager::stopRemoveNode()
{
	logger().information(
		"stopping the Z-Wave node removal process",
		__FILE__, __LINE__);

	try {
		m_network->cancelRemoveNode();
	}
	catch (const IllegalStateException &e) {
		logger().warning(e.displayText(), __FILE__, __LINE__);
	}
	BEEEON_CATCH_CHAIN(logger())
}

AsyncWork<double>::Ptr ZWaveDeviceManager::startSetValue(
		const DeviceID &id,
		const ModuleID &module,
		const double value,
		const Poco::Timespan &timeout)
{
	FastMutex::ScopedLock guard(m_lock, timeout.totalMilliseconds());

	auto it = m_devices.find(id);
	if (it == m_devices.end())
		throw NotFoundException("no such device " + id.toString() + " to set value");

	Device &device = it->second;
	auto mapper = device.mapper();

	m_network->postValue(mapper->convert(module, value));

	auto work = BlockingAsyncWork<double>::instance();
	work->setResult(value);

	return work;
}
