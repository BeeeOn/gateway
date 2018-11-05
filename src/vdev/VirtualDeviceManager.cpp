#include <sstream>

#include <Poco/NumberParser.h>
#include <Poco/Util/AbstractConfiguration.h>

#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "commands/NewDeviceCommand.h"

#include "core/CommandDispatcher.h"
#include "core/Distributor.h"
#include "di/Injectable.h"
#include "model/SensorData.h"
#include "vdev/VirtualDeviceManager.h"

BEEEON_OBJECT_BEGIN(BeeeOn, VirtualDeviceManager)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(DeviceStatusHandler)
BEEEON_OBJECT_PROPERTY("deviceCache", &VirtualDeviceManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("file", &VirtualDeviceManager::setConfigFile)
BEEEON_OBJECT_PROPERTY("distributor", &VirtualDeviceManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &VirtualDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_HOOK("done", &VirtualDeviceManager::installVirtualDevices)
BEEEON_OBJECT_END(BeeeOn, VirtualDeviceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Util;
using namespace std;

const static unsigned int DEFAULT_REFRESH_SECS = 30;

VirtualDeviceManager::VirtualDeviceManager():
	DeviceManager(DevicePrefix::PREFIX_VIRTUAL_DEVICE, {
		typeid(GatewayListenCommand),
		typeid(DeviceAcceptCommand),
		typeid(DeviceUnpairCommand),
		typeid(DeviceSetValueCommand),
	})
{
}

void VirtualDeviceManager::registerDevice(
	const VirtualDevice::Ptr device)
{
	if (!m_virtualDevicesMap.emplace(device->deviceID(), device).second) {
		throw ExistsException("registering duplicate device: "
			+ device->deviceID().toString());
	}

	logger().debug(
		"registering new virtual device "
		+ device->deviceID().toString(),
		__FILE__, __LINE__
	);
}

void VirtualDeviceManager::logDeviceParsed(VirtualDevice::Ptr device)
{
	logger().information(
		"virtual device: "
		+ device->deviceID().toString(),
		__FILE__, __LINE__
	);

	logger().debug(
		"virtual device: "
		+ device->deviceID().toString()
		+ ", modules: "
		+ to_string(device->modules().size())
		+ ", paired: "
		+ (deviceCache()->paired(device->deviceID()) ? "yes" : "no")
		+ ", refresh: "
		+ device->refresh().toString()
		+ ", vendor: "
		+ device->vendorName()
		+ ", product: "
		+ device->productName(),
		__FILE__, __LINE__
	);

	for (auto &module : device->modules()) {
		logger().trace(
			"virtual device: "
			+ device->deviceID().toString()
			+ ", module: "
			+ module->moduleID().toString()
			+ ", type: "
			+ module->moduleType().type().toString(),
			__FILE__, __LINE__
		);
	}
}

VirtualDevice::Ptr VirtualDeviceManager::parseDevice(
	AutoPtr <AbstractConfiguration> cfg)
{
	VirtualDevice::Ptr device = new VirtualDevice;

	DeviceID id = DeviceID::parse(cfg->getString("device_id"));
	if (id.prefix() != DevicePrefix::PREFIX_VIRTUAL_DEVICE) {
		device->setDeviceId(
			DeviceID(DevicePrefix::PREFIX_VIRTUAL_DEVICE, id.ident()));

		logger().warning(
			"device prefix was wrong, overriding ID to "
			+ device->deviceID().toString(),
			__FILE__, __LINE__
		);
	}
	else {
		device->setDeviceId(id);
	}

	unsigned int refresh = cfg->getUInt("refresh", DEFAULT_REFRESH_SECS);
	device->setRefresh(RefreshTime::fromSeconds(refresh));

	if (cfg->getBool("paired", false))
		deviceCache()->markPaired(id);
	else
		deviceCache()->markUnpaired(id);

	device->setVendorName(cfg->getString("vendor"));
	device->setProductName(cfg->getString("product"));

	for (int i = 0; cfg->has("module" + to_string(i) + ".type"); ++i) {
		try {
			AutoPtr<AbstractConfiguration> view =
				cfg->createView("module" + to_string(i));
			device->addModule(parseModule(view, i));
		}
		catch (const Exception &ex) {
			logger().log(ex, __FILE__, __LINE__);
			logger().critical(
				"failed to initialize module "
				+ id.toString(),
				__FILE__, __LINE__
			);
			break;
		}
	}
	logDeviceParsed(device);

	return device;
}

VirtualModule::Ptr VirtualDeviceManager::parseModule(
	AutoPtr<AbstractConfiguration> cfg,
	const ModuleID &moduleID)
{
	ModuleType type = ModuleType::parse(cfg->getString("type"));
	VirtualModule::Ptr virtualModule = new VirtualModule(type);

	virtualModule->setModuleID(moduleID);
	virtualModule->setMin(cfg->getDouble("min", 0));
	virtualModule->setMax(cfg->getDouble("max", 100));
	virtualModule->setGenerator(cfg->getString("generator", ""));
	virtualModule->setReaction(cfg->getString("reaction", "none"));

	return virtualModule;
}

void VirtualDeviceManager::setConfigFile(const string &file)
{
	m_configFile = file;
}

void VirtualDeviceManager::installVirtualDevices()
{
	logger().information("loading configuration from: " + m_configFile);
	AutoPtr<AbstractConfiguration> cfg = new IniFileConfiguration(m_configFile);

	m_requestDeviceList =
		cfg->getBool("virtual-devices.request.device_list", true);

	for (int i = 0; cfg->has("virtual-device" + to_string(i) + ".enable"); ++i) {
		const string &prefix = "virtual-device" + to_string(i);

		if (!cfg->getBool(prefix + ".enable", false))
			continue;

		try {
			VirtualDevice::Ptr device = parseDevice(
				cfg->createView(prefix));
			registerDevice(device);
		}
		catch (const Exception &ex) {
			logger().log(ex, __FILE__, __LINE__);
			logger().error(
				"virtual device was not parsed or registered successfully",
				__FILE__, __LINE__
			);
			continue;
		}
	}

	logger().information(
		"loaded "
		+ to_string(m_virtualDevicesMap.size())
		+ " virtual devices",
		__FILE__, __LINE__
	);
}

void VirtualDeviceManager::dispatchNewDevice(VirtualDevice::Ptr device)
{
	const auto description = DeviceDescription::Builder()
		.id(device->deviceID())
		.type(device->vendorName(), device->productName())
		.modules(device->moduleTypes())
		.refreshTime(device->refresh())
		.build();

	dispatch(new NewDeviceCommand(description));
}

void VirtualDeviceManager::doListenCommand(
	const GatewayListenCommand::Ptr)
{
	FastMutex::ScopedLock guard(m_lock);
	for (auto &item : m_virtualDevicesMap) {
		if (!deviceCache()->paired(item.first))
			dispatchNewDevice(item.second);
	}
}

void VirtualDeviceManager::doDeviceAcceptCommand(
		const DeviceAcceptCommand::Ptr cmd)
{
	FastMutex::ScopedLock guard(m_lock);
	auto it = m_virtualDevicesMap.find(cmd->deviceID());
	if (it == m_virtualDevicesMap.end())
		throw NotFoundException("accept " + cmd->deviceID().toString());

	if (deviceCache()->paired(cmd->deviceID())) {
		logger().warning(
			"ignoring accept for paired device "
			+ cmd->deviceID().toString(),
			__FILE__, __LINE__
		);
	}

	const VirtualDeviceEntry entry(it->second);
	deviceCache()->markPaired(cmd->deviceID());
	scheduleEntryUnlocked(entry);
}

void VirtualDeviceManager::doUnpairCommand(
		const DeviceUnpairCommand::Ptr cmd)
{
	FastMutex::ScopedLock guard(m_lock);
	auto it = m_virtualDevicesMap.find(cmd->deviceID());
	if (it == m_virtualDevicesMap.end()) {
		logger().warning(
			"unpairing device that is not registered: "
			+ cmd->deviceID().toString(),
			__FILE__, __LINE__
		);

		return;
	}

	if (!deviceCache()->paired(cmd->deviceID())) {
		logger().warning(
			"unpairing device that is not paired: "
			+ cmd->deviceID().toString(),
			__FILE__, __LINE__
		);
	}

	deviceCache()->markUnpaired(cmd->deviceID());
}

void VirtualDeviceManager::doSetValueCommand(
	const DeviceSetValueCommand::Ptr cmd)
{
	FastMutex::ScopedLock guard(m_lock);
	auto it = m_virtualDevicesMap.find(cmd->deviceID());
	if (it == m_virtualDevicesMap.end())
		throw NotFoundException("set-value: " + cmd->deviceID().toString());

	for (auto &item : it->second->modules()) {
		if (item->moduleID() == cmd->moduleID()) {
			if (item->reaction() == VirtualModule::REACTION_NONE) {
				throw InvalidAccessException(
					"cannot set-value: " + cmd->deviceID().toString());
			}
		}
	}

	if (!it->second->modifyValue(cmd->moduleID(), cmd->value())) {
		throw IllegalStateException(
			"set-value: " + cmd->deviceID().toString());
	}

	logger().debug(
		"module "
		+ cmd->moduleID().toString()
		+ " is set to value "
		+ to_string(cmd->value()),
		__FILE__, __LINE__
	);
}

void VirtualDeviceManager::handleGeneric(const Command::Ptr cmd, Result::Ptr result)
{
	if (cmd->is<GatewayListenCommand>())
		doListenCommand(cmd.cast<GatewayListenCommand>());
	else if (cmd->is<DeviceSetValueCommand>())
		doSetValueCommand(cmd.cast<DeviceSetValueCommand>());
	else if (cmd->is<DeviceUnpairCommand>())
		doUnpairCommand(cmd.cast<DeviceUnpairCommand>());
	else if (cmd->is<DeviceAcceptCommand>())
		doDeviceAcceptCommand(cmd.cast<DeviceAcceptCommand>());
	else
		DeviceManager::handleGeneric(cmd, result);
}

void VirtualDeviceManager::handleRemoteStatus(
	const DevicePrefix &prefix,
	const set<DeviceID> &devices,
	const DeviceStatusHandler::DeviceValues &values)
{
	DeviceManager::handleRemoteStatus(prefix, devices, values);
	scheduleAllEntries();
}

void VirtualDeviceManager::scheduleAllEntries()
{
	FastMutex::ScopedLock guard(m_lock);
	for (auto &item : m_virtualDevicesMap) {
		if (deviceCache()->paired(item.first)) {
			const VirtualDeviceEntry entry(item.second);
			scheduleEntryUnlocked(VirtualDeviceEntry(item.second));
		}
	}
}

bool VirtualDeviceManager::isEmptyQueue()
{
	FastMutex::ScopedLock guard(m_lock);
	return m_virtualDeviceQueue.empty();
}

void VirtualDeviceManager::run()
{
	scheduleAllEntries();

	StopControl::Run run(m_stopControl);

	while (run) {
		if (isEmptyQueue()) {
			logger().debug(
				"empty queue of devices",
				__FILE__, __LINE__);
			run.waitStoppable(-1);
			continue;
		}

		ScopedLockWithUnlock<FastMutex> guard(m_lock);
		const VirtualDeviceEntry entry = m_virtualDeviceQueue.top();

		if (!deviceCache()->paired(entry.device()->deviceID())) {
			logger().debug(
				"unpaired device "
				+ entry.device()->deviceID().toString()
				+ " was removed from queue",
				__FILE__, __LINE__);
			m_virtualDeviceQueue.pop();
			continue;
		}

		Timestamp now;
		const Timestamp &activationTime = entry.activationTime();
		const Timespan sleepTime(activationTime - now);
		if (sleepTime.totalMilliseconds() > 0) {
			guard.unlock();
			logger().debug(
				"device "
				+ entry.device()->deviceID().toString()
				+ " will be activated in "
				+ to_string(sleepTime.totalMilliseconds())
				+ " milliseconds",
				__FILE__, __LINE__);
			run.waitStoppable(sleepTime);
			continue;
		}

		m_virtualDeviceQueue.pop();

		logger().debug(
			"device "
			+ entry.device()->deviceID().toString()
			+ " is being processed",
			__FILE__, __LINE__);

		SensorData sensorData = entry.device()->generate();
		if (!sensorData.isEmpty()) {
			ship(entry.device()->generate());
		}
		else {
			poco_debug(logger(), "received empty SensorData");
		}

		scheduleEntryUnlocked(entry);
	}
}

void VirtualDeviceManager::stop()
{
	DeviceManager::stop();
	answerQueue().dispose();
}

void VirtualDeviceManager::scheduleEntryUnlocked(VirtualDeviceEntry entry)
{
	entry.setInserted(Timestamp());
	m_virtualDeviceQueue.push(entry);
	m_stopControl.requestWakeup();
}

bool VirtualDeviceEntryComparator::lessThan(
	const VirtualDeviceEntry &deviceQueue1,
	const VirtualDeviceEntry &deviceQueue2) const
{
	return deviceQueue1.activationTime() > deviceQueue2.activationTime();
}

VirtualDeviceEntry::VirtualDeviceEntry(VirtualDevice::Ptr device)
{
	m_device = device;
}

void VirtualDeviceEntry::setInserted(const Timestamp &t)
{
	m_inserted = t;
}

Timestamp VirtualDeviceEntry::inserted() const
{
	return m_inserted;
}

VirtualDevice::Ptr VirtualDeviceEntry::device() const
{
	return m_device;
}

Timestamp VirtualDeviceEntry::activationTime() const
{
	return inserted() + device()->refresh().time();
}
