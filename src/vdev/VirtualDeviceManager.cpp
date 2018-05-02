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
	DeviceManager(DevicePrefix::PREFIX_VIRTUAL_DEVICE)
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
		+ (device->paired() ? "yes" : "no")
		+ ", refresh: "
		+ to_string(device->refresh().totalSeconds())
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
	device->setRefresh(refresh * Timespan::SECONDS);

	device->setPaired(cfg->getBool("paired", false));
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

bool VirtualDeviceManager::accept(const Command::Ptr cmd)
{
	if (cmd->is<GatewayListenCommand>()) {
		return true;
	}
	else if (cmd->is<DeviceSetValueCommand>()) {
		return cmd->cast<DeviceSetValueCommand>().deviceID().prefix()
			== DevicePrefix::PREFIX_VIRTUAL_DEVICE;
	}
	else if (cmd->is<DeviceUnpairCommand>()) {
		return cmd->cast<DeviceUnpairCommand>().deviceID().prefix()
			== DevicePrefix::PREFIX_VIRTUAL_DEVICE;
	}
	else if (cmd->is<DeviceAcceptCommand>()) {
		return cmd->cast<DeviceAcceptCommand>().deviceID().prefix()
			== DevicePrefix::PREFIX_VIRTUAL_DEVICE;
	}

	return false;
}

void VirtualDeviceManager::dispatchNewDevice(VirtualDevice::Ptr device)
{
	NewDeviceCommand::Ptr cmd = new NewDeviceCommand(
		device->deviceID(),
		device->vendorName(),
		device->productName(),
		device->moduleTypes(),
		device->refresh());

	dispatch(cmd);
}

void VirtualDeviceManager::doListenCommand(
	const GatewayListenCommand::Ptr, const Answer::Ptr answer)
{
	Result::Ptr result = new Result(answer);

	FastMutex::ScopedLock guard(m_lock);
	for (auto &item : m_virtualDevicesMap) {
		if (!item.second->paired())
			dispatchNewDevice(item.second);
	}

	result->setStatus(Result::Status::SUCCESS);
}

void VirtualDeviceManager::doDeviceAcceptCommand(
	const DeviceAcceptCommand::Ptr cmd, const Answer::Ptr answer)
{
	Result::Ptr result = new Result(answer);

	FastMutex::ScopedLock guard(m_lock);
	auto it = m_virtualDevicesMap.find(cmd->deviceID());
	if (it == m_virtualDevicesMap.end()) {
		logger().warning(
			"no such device "
			+ cmd->deviceID().toString()
			+ " to accept",
			__FILE__, __LINE__
		);

		result->setStatus(Result::Status::FAILED);
		return;
	}

	if (it->second->paired()) {
		logger().warning(
			"ignoring accept for paired device "
			+ cmd->deviceID().toString(),
			__FILE__, __LINE__
		);
	}

	const VirtualDeviceEntry entry(it->second);
	entry.device()->setPaired(true);
	scheduleEntryUnlocked(entry);

	result->setStatus(Result::Status::SUCCESS);
}

void VirtualDeviceManager::doUnpairCommand(
	const DeviceUnpairCommand::Ptr cmd, const Answer::Ptr answer)
{
	Result::Ptr result = new Result(answer);

	FastMutex::ScopedLock guard(m_lock);
	auto it = m_virtualDevicesMap.find(cmd->deviceID());
	if (it == m_virtualDevicesMap.end()) {
		logger().warning(
			"unpairing device that is not registered: "
			+ cmd->deviceID().toString(),
			__FILE__, __LINE__
		);

		result->setStatus(Result::Status::SUCCESS);
		return;
	}

	if (!it->second->paired()) {
		logger().warning(
			"unpairing device that is not paired: "
			+ cmd->deviceID().toString(),
			__FILE__, __LINE__
		);
	}

	it->second->setPaired(false);

	result->setStatus(Result::Status::SUCCESS);
}

void VirtualDeviceManager::doSetValueCommand(
	const DeviceSetValueCommand::Ptr cmd, const Answer::Ptr answer)
{
	Result::Ptr result = new Result(answer);

	FastMutex::ScopedLock guard(m_lock);
	auto it = m_virtualDevicesMap.find(cmd->deviceID());
	if (it == m_virtualDevicesMap.end()) {
		logger().warning(
			"failed to set value for non-existing device "
			+ cmd->deviceID().toString(),
			__FILE__, __LINE__
		);

		result->setStatus(Result::Status::FAILED);
		return;
	}

	for (auto &item : it->second->modules()) {
		if (item->moduleID() == cmd->moduleID()) {
			if (item->reaction() == VirtualModule::REACTION_NONE) {
				logger().warning(
					"value of module "
					+ item->moduleType().type().toString()
					+ " cannot be set");

				result->setStatus(Result::Status::FAILED);
				return;
			}
		}
	}

	if (it->second->modifyValue(cmd->moduleID(), cmd->value()))
		result->setStatus(Result::Status::SUCCESS);
	else
		result->setStatus(Result::Status::FAILED);

	logger().debug(
		"module "
		+ cmd->moduleID().toString()
		+ " is set to value "
		+ to_string(cmd->value()),
		__FILE__, __LINE__
	);
}

void VirtualDeviceManager::handle(Command::Ptr cmd, Answer::Ptr answer)
{
	if (cmd->is<GatewayListenCommand>())
		doListenCommand(cmd.cast<GatewayListenCommand>(), answer);
	else if (cmd->is<DeviceSetValueCommand>())
		doSetValueCommand(cmd.cast<DeviceSetValueCommand>(), answer);
	else if (cmd->is<DeviceUnpairCommand>())
		doUnpairCommand(cmd.cast<DeviceUnpairCommand>(), answer);
	else if (cmd->is<DeviceAcceptCommand>())
		doDeviceAcceptCommand(cmd.cast<DeviceAcceptCommand>(), answer);
	else
		throw IllegalStateException("received unaccepted command");
}

void VirtualDeviceManager::setPairedDevices()
{
	if (m_requestDeviceList) {
		set<DeviceID> deviceIDs = deviceList(-1);

		FastMutex::ScopedLock guard(m_lock);
		for (auto &pair : m_virtualDevicesMap) {
			if (deviceIDs.find(pair.first) != deviceIDs.end())
				continue;

			pair.second->setPaired(true);
		}
	}
	else {
		logger().information("skipping request of device list");
	}
}

void VirtualDeviceManager::scheduleAllEntries()
{
	FastMutex::ScopedLock guard(m_lock);
	for (auto &item : m_virtualDevicesMap) {
		if (item.second->paired()) {
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
	setPairedDevices();
	scheduleAllEntries();

	while (!m_stop) {
		if (isEmptyQueue()) {
			logger().debug(
				"empty queue of devices",
				__FILE__, __LINE__);
			m_event.wait();
			continue;
		}

		ScopedLockWithUnlock<FastMutex> guard(m_lock);
		const VirtualDeviceEntry entry = m_virtualDeviceQueue.top();

		if (!entry.device()->paired()) {
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
			m_event.tryWait(sleepTime.totalMilliseconds());
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
		m_event.reset();
	}
}

void VirtualDeviceManager::stop()
{
	m_stop = true;
	m_event.set();
	answerQueue().dispose();
}

void VirtualDeviceManager::scheduleEntryUnlocked(VirtualDeviceEntry entry)
{
	entry.setInserted(Timestamp());
	m_virtualDeviceQueue.push(entry);
	m_event.set();
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
	return inserted() + device()->refresh();
}
