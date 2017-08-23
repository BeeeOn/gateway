#include <list>

#include <Poco/Logger.h>
#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>

#include <openzwave/Manager.h>
#include <openzwave/Options.h>

#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "hotplug/HotplugEvent.h"
#include "model/SensorData.h"
#include "zwave/GenericZWaveDeviceInfoRegistry.h"
#include "zwave/ZWaveDeviceManager.h"
#include "zwave/ZWavePocoLoggerAdapter.h"

BEEEON_OBJECT_BEGIN(BeeeOn, ZWaveDeviceManager)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(HotplugListener)
BEEEON_OBJECT_TEXT("userPath", &ZWaveDeviceManager::setUserPath)
BEEEON_OBJECT_TEXT("configPath", &ZWaveDeviceManager::setConfigPath)
BEEEON_OBJECT_NUMBER("pollInterval", &ZWaveDeviceManager::setPollInterval)
BEEEON_OBJECT_REF("commandDispatcher", &ZWaveDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_REF("distributor", &ZWaveDeviceManager::setDistributor)
BEEEON_OBJECT_REF("deviceInfoRegistry", &ZWaveDeviceManager::setDeviceInfoRegistry)
BEEEON_OBJECT_HOOK("done", &ZWaveDeviceManager::installConfiguration);
BEEEON_OBJECT_END(BeeeOn, ZWaveDeviceManager)

using namespace BeeeOn;
using namespace OpenZWave;
using namespace std;

using Poco::FastMutex;
using Poco::Nullable;
using Poco::NumberFormatter;
using Poco::SharedPtr;
using Poco::Timer;

static const int NODE_ID_MASK = 0xff;
static const int DEFAULT_UNPAIR_TIMEOUT_MS = 5000;

static const int FAILS_TRESHOLD = 3;
static const Poco::Timespan SLEEP_AFTER_FAILS = 10 * Poco::Timespan::SECONDS;
static const Poco::Timespan SLEEP_BETWEEN_FAILS = 1 * Poco::Timespan::SECONDS;

/**
 * The OpenZWave library uses a notification loop to provide information
 * about the Z-Wave network. A notification represents e.g. detection of a
 * new device, change of a value, Z-Wave dongle initialization, etc.
 */
static void onNotification(
	Notification const *notification, void *context)
{
	ZWaveDeviceManager *processor =
		reinterpret_cast<ZWaveDeviceManager *>(context);
	processor->onNotification(notification);
}

ZWaveDeviceManager::ZWaveDeviceManager():
	DeviceManager(DevicePrefix::PREFIX_ZWAVE),
	m_state(State::IDLE),
	m_commandCallback(*this, &ZWaveDeviceManager::stopCommand),
	m_commandTimer(0, 0)
{
}

ZWaveDeviceManager::~ZWaveDeviceManager()
{
	Manager::Destroy();
	Options::Destroy();
}

void ZWaveDeviceManager::installConfiguration()
{
	Options::Create(m_configPath, m_userPath, "");
	Options::Get()->AddOptionInt("PollInterval", m_pollInterval);
	Options::Get()->AddOptionBool("logging", true);
	Options::Get()->AddOptionBool("SaveConfiguration", false);
	Options::Get()->Lock();

	Manager::Create();

	// pocoLogger is deleted by OpenZWave library
	ZWavePocoLoggerAdapter *pocoLogger =
		new ZWavePocoLoggerAdapter(Loggable::forMethod("OpenZWaveLibrary"));
	Log::SetLoggingClass(pocoLogger);
}

void ZWaveDeviceManager::run()
{
	m_stopEvent.wait();
}

string ZWaveDeviceManager::dongleMatch(const HotplugEvent &e)
{
	if (!e.properties()->has("tty.BEEEON_DONGLE"))
		return "";

	const string &dongleType = e.properties()
			->getString("tty.BEEEON_DONGLE");

	if (dongleType != "zwave")
		return "";

	return e.node();
}

void ZWaveDeviceManager::onAdd(const HotplugEvent &event)
{
	FastMutex::ScopedLock guard(m_dongleLock);

	if (!m_driver.driverPath().empty()) {
		logger().trace("ignored event " + event.toString(),
			__FILE__, __LINE__);
		return;
	}

	const string &name = dongleMatch(event);
	if (name.empty()) {
		logger().trace("event " + event.toString() + " does not match",
			__FILE__, __LINE__);
		return;
	}

	logger().debug("registering dongle " + event.toString(),
		__FILE__, __LINE__);

	m_state = State::IDLE;
	m_driver.setDriverPath(name);

	Manager::Get()->AddWatcher(::onNotification, this);
	m_driver.registerItself();
}

void ZWaveDeviceManager::onRemove(const HotplugEvent &event)
{
	FastMutex::ScopedLock guard(m_dongleLock);

	if (m_driver.driverPath().empty()) {
		logger().debug("ignored event " + event.toString(),
			__FILE__, __LINE__);
		return;
	}

	const string &name = dongleMatch(event);
	if (name.empty()) {
		logger().debug("event " + event.toString() + " does not match",
			__FILE__, __LINE__);
		return;
	}

	if (event.node() != m_driver.driverPath()) {
		logger().trace("ignored remove dongle: " + event.toString(),
			__FILE__, __LINE__);
		return;
	}

	logger().debug("unregistering dongle " + event.toString(),
		__FILE__, __LINE__);

	m_state = State::IDLE;
	m_driver.unregisterItself();
	m_driver.setDriverPath("");

	Manager::Get()->RemoveWatcher(::onNotification, this);
}

bool ZWaveDeviceManager::accept(const Command::Ptr cmd)
{
	if (cmd->is<GatewayListenCommand>() ) {
		return !m_driver.driverPath().empty();
	}
	else if (cmd->is<DeviceUnpairCommand>()) {
		DeviceID deviceID = cmd->cast<DeviceUnpairCommand>().deviceID();
		return deviceID.prefix() == DevicePrefix::PREFIX_ZWAVE;
	}
	else if (cmd->is<DeviceAcceptCommand>()) {
		DeviceID deviceID = cmd->cast<DeviceAcceptCommand>().deviceID();
		return deviceID.prefix() == DevicePrefix::PREFIX_ZWAVE;
	}
	else if (cmd->is<DeviceSetValueCommand>()) {
		DeviceID deviceID = cmd->cast<DeviceSetValueCommand>().deviceID();
		return deviceID.prefix() == DevicePrefix::PREFIX_ZWAVE;
	}

	return false;
}

void ZWaveDeviceManager::handle(Command::Ptr cmd, Answer::Ptr answer)
{
	if (cmd->is<GatewayListenCommand>())
		doListenCommand(cmd.cast<GatewayListenCommand>(), answer);
	else if (cmd->is<DeviceUnpairCommand>())
		doUnpairCommand(cmd.cast<DeviceUnpairCommand>(), answer);
	else if (cmd->is<DeviceAcceptCommand>())
		doDeviceAcceptCommand(cmd.cast<DeviceAcceptCommand>(), answer);
	else if (cmd->is<DeviceSetValueCommand>())
		doSetValueCommnad(cmd.cast<DeviceSetValueCommand>(), answer);
}

void ZWaveDeviceManager::doListenCommand(
	const GatewayListenCommand::Ptr cmd, const Answer::Ptr answer)
{
	FastMutex::ScopedLock guard(m_lock);
	Result::Ptr result = new Result(answer);

	if (m_state != State::NODE_QUERIED) {
		logger().error(
			"device manager is not in queried state",
			__FILE__, __LINE__
		);

		result->setStatus(Result::Status::FAILED);
		return;
	}

	m_state = LISTENING;
	logger().debug(
		"start listening mode for "
		+ to_string(cmd->duration().totalSeconds())
		+ " seconds",
		__FILE__, __LINE__);

	if (Manager::Get()->AddNode(m_homeID))
		result->setStatus(Result::Status::SUCCESS);
	else
		result->setStatus(Result::Status::FAILED);

	m_commandTimer.stop();
	m_commandTimer.setStartInterval(cmd->duration().totalMilliseconds());
	m_commandTimer.start(m_commandCallback);
}

void ZWaveDeviceManager::doUnpairCommand(
	const DeviceUnpairCommand::Ptr cmd, const Answer::Ptr answer)
{
	FastMutex::ScopedLock guard(m_lock);
	Result::Ptr result = new Result(answer);

	if (m_state != State::NODE_QUERIED) {
		logger().error(
			"device manager is not in queried state",
			__FILE__, __LINE__);

		result->setStatus(Result::Status::FAILED);
		return;
	}

	m_state = UNPAIRING;
	logger().debug(
		"unpairing device "
		+ cmd->deviceID().toString()
		+ " with timeout "
		+ to_string(DEFAULT_UNPAIR_TIMEOUT_MS / 1000)
		+ " seconds",
		__FILE__, __LINE__);

	uint8_t nodeID = nodeIDFromDeviceID(cmd->deviceID());
	auto it = m_beeeonDevices.find(nodeID);
	if (it == m_beeeonDevices.end()) {
		result->setStatus(Result::Status::SUCCESS);
		return;
	}

	m_beeeonDevices.erase(nodeID);

	if (Manager::Get()->RemoveNode(m_homeID))
		result->setStatus(Result::Status::SUCCESS);
	else
		result->setStatus(Result::Status::FAILED);

	m_commandTimer.stop();
	m_commandTimer.setStartInterval(DEFAULT_UNPAIR_TIMEOUT_MS);
	m_commandTimer.start(m_commandCallback);
}

void ZWaveDeviceManager::stopCommand(Timer &)
{
	Manager::Get()->CancelControllerCommand(m_homeID);
	m_state = NODE_QUERIED;

	logger().debug("command is done", __FILE__, __LINE__);
}

void ZWaveDeviceManager::doDeviceAcceptCommand(
	const DeviceAcceptCommand::Ptr cmd, const Answer::Ptr answer)
{
	FastMutex::ScopedLock guard(m_lock);
	Result::Ptr result = new Result(answer);

	uint8_t nodeID = nodeIDFromDeviceID(cmd->deviceID());

	auto it = m_beeeonDevices.find(nodeID);
	if (it == m_beeeonDevices.end()) {
		result->setStatus(Result::Status::FAILED);
		logger().warning(
			"no such device "
			+ cmd->deviceID().toString()
			+ " (" + to_string(nodeID)
			+ ") to accept",
			__FILE__, __LINE__);

		return;
	}

	it->second.setPaired(true);
	result->setStatus(Result::Status::SUCCESS);

	logger().trace(
		"device "
		+ cmd->deviceID().toString()
		+ " is paired",
		__FILE__, __LINE__);
}

void ZWaveDeviceManager::doSetValueCommnad(
	const DeviceSetValueCommand::Ptr cmd, const Answer::Ptr answer)
{
	FastMutex::ScopedLock guard(m_lock);
	Result::Ptr result = new Result(answer);

	if (m_state < State::NODE_QUERIED) {
		logger().error(
			"device manager is not in queried state",
			__FILE__, __LINE__
		);

		result->setStatus(Result::Status::FAILED);
		return;
	}

	uint8_t nodeID = nodeIDFromDeviceID(cmd->deviceID());
	auto it = m_beeeonDevices.find(nodeID);
	if (it == m_beeeonDevices.end()) {
		result->setStatus(Result::Status::FAILED);
		return;
	}

	bool ret = false;
	ZWaveDeviceInfo::Ptr info;

	try {
		info = m_registry->find(
			it->second.vendorID(), it->second.productID());

		string value = info->convertValue(cmd->value());

		ret = modifyValue(nodeID, cmd->moduleID(), value);
	}
	catch (const Poco::Exception &ex) {
		logger().log(ex, __FILE__, __LINE__);
		logger().error(
			"failed to set value for device "
			+ cmd->deviceID().toString(),
			__FILE__, __LINE__);
	}

	if (ret)
		result->setStatus(Result::Status::SUCCESS);
	else
		result->setStatus(Result::Status::FAILED);
}

void ZWaveDeviceManager::valueAdded(const Notification *notification)
{
	if (m_state != DONGLE_READY && m_state != LISTENING) {
		logger().error(
			"device manager does not have added driver or it is not in listening state",
			__FILE__, __LINE__
		);

		return;
	}

	uint8_t nodeID = notification->GetNodeId();

	auto it = m_zwaveNodes.find(nodeID);
	if (it == m_zwaveNodes.end()) {
		logger().warning(
			"unknown node ID: "
			+ nodeID,
			__FILE__, __LINE__);

		return;
	}

	it->second.push_back(notification->GetValueID());
}

void ZWaveDeviceManager::valueChanged(const Notification *notification)
{
	if (m_state < NODE_QUERIED)
		return;

	uint8_t nodeID = notification->GetNodeId();

	auto it = m_beeeonDevices.find(nodeID);
	if (it == m_beeeonDevices.end() || !it->second.paired()) {
		logger().debug(
			"device: "
			+ buildID(nodeID).toString()
			+ " is not paired",
			__FILE__, __LINE__);
		return;
	}

	try {
		shipData(notification->GetValueID(), it->second);
	}
	catch (const Poco::Exception &ex) {
		logger().log(ex, __FILE__, __LINE__);
		logger().error(
			"failed to ship data for node "
			+ to_string(nodeID),
			__FILE__, __LINE__);
	}
}

void ZWaveDeviceManager::shipData(const ValueID &valueID, const ZWaveNodeInfo &nodeInfo)
{
	SensorData sensorData;
	sensorData.setDeviceID(nodeInfo.deviceID());

	ZWaveDeviceInfo::Ptr info = m_registry->find(
		nodeInfo.vendorID(), nodeInfo.productID());

	if (!info->registry()->contains(valueID.GetCommandClassId(), valueID.GetIndex())) {
		logger().trace(
			"unsupported value with command class "
			+ to_string(valueID.GetCommandClassId())
			+ " and index "
			+ to_string(valueID.GetIndex()),
			__FILE__, __LINE__);
		return;
	}

	sensorData.insertValue(
		SensorValue(
			nodeInfo.findModuleID(valueID),
			info->extractValue(valueID, nodeInfo.findModuleType(valueID))
		)
	);

	ship(sensorData);
}

void ZWaveDeviceManager::nodeAdded(
	const Notification *notification)
{
	if (m_state != DONGLE_READY && m_state != LISTENING) {
		logger().error(
			"device manager does not have added driver or it is not in listening state",
			__FILE__, __LINE__
		);

		return;
	}

	uint8_t nodeID = notification->GetNodeId();
	auto it = m_zwaveNodes.emplace(nodeID, list<OpenZWave::ValueID>());

	if (!it.second) {
		logger().debug(
			"node ID "
			+ to_string(nodeID)
			+ " is already registered",
			__FILE__, __LINE__
		);
	}
}

void ZWaveDeviceManager::doNewDeviceCommand()
{
	if (m_state != State::LISTENING) {
		logger().warning(
			"attempt to send new devices out of listening mode",
			__FILE__, __LINE__);

		return;
	}

	for (auto &node : m_zwaveNodes) {
		try {
			createDevice(buildID(node.first), false);
		}
		catch (const Poco::ExistsException &ex) {
			logger().log(ex, __FILE__, __LINE__);
		}
	}

	for (auto &nodeID : m_beeeonDevices) {
		if (nodeID.second.paired())
			continue;

		if (nodeID.second.vendorName().empty() && nodeID.second.productName().empty()) {
			logger().trace(
				"no info about node id" + to_string(nodeID.first),
				__FILE__, __LINE__
			);
			continue;
		}

		dispatch(
			new NewDeviceCommand(
				buildID(nodeID.first),
				nodeID.second.vendorName(),
				nodeID.second.productName(),
				nodeID.second.moduleTypes()
			)
		);
	}
}

void ZWaveDeviceManager::onNotification(
	const Notification *notification)
{
	FastMutex::ScopedLock guard(m_lock);
	auto it = m_beeeonDevices.find(notification->GetNodeId());

	switch (notification->GetType()) {
	case Notification::Type_ValueAdded:
		valueAdded(notification);
		break;
	case Notification::Type_ValueChanged:
		valueChanged(notification);
		break;
	case Notification::Type_NodeAdded:
		nodeAdded(notification);
		Manager::Get()->WriteConfig(m_homeID);
		break;
	case Notification::Type_NodeRemoved:
		Manager::Get()->WriteConfig(m_homeID);
		break;
	case Notification::Type_NodeQueriesComplete:
		doNewDeviceCommand();
		Manager::Get()->WriteConfig(m_homeID);
		break;
	case Notification::Type_PollingDisabled: {
		if (it != m_beeeonDevices.end())
			it->second.setPolled(false);

		break;
	}
	case Notification::Type_PollingEnabled: {
		if (it != m_beeeonDevices.end())
			it->second.setPolled(true);

		break;
	}
	case Notification::Type_DriverReady: {
		m_homeID = notification->GetHomeId();
		m_state = DONGLE_READY;

		m_beeeonDevices.clear();
		m_zwaveNodes.clear();

		logger().notice(
			"driver: " + m_driver.driverPath() + " ready "
			+ "(home ID: "
			+ NumberFormatter::formatHex(m_homeID, true)
			+ ")",
			__FILE__, __LINE__);
		break;
	}
	case Notification::Type_DriverFailed: {
		m_state = State::IDLE;

		logger().critical(
			"driver: "
			+ m_driver.driverPath()
			+ " failed",
			__FILE__, __LINE__);
		break;
	}
	case Notification::Type_AwakeNodesQueried:
	case Notification::Type_AllNodesQueried:
	case Notification::Type_AllNodesQueriedSomeDead:
		if (m_state != DONGLE_READY) {
			logger().warning(
				"driver " + m_driver.driverPath()
				+ " is not ready for use",
				__FILE__, __LINE__);
		}

		m_state = NODE_QUERIED;

		logger().debug(
			"all nodes queried",
			__FILE__, __LINE__);

		Manager::Get()->WriteConfig(m_homeID);

		loadDeviceList();
		break;
	default:
		break;
	}
}

void ZWaveDeviceManager::loadDeviceList()
{
	set<DeviceID> deviceIDs;

	int fails = 0;
	while (!m_stop) {
		try {
			deviceIDs = deviceList(-1);
			break;
		}
		catch (const Poco::Exception &ex) {
			logger().log(ex, __FILE__, __LINE__);

			if (fails >= FAILS_TRESHOLD) {
				fails = 0;
				Poco::Thread::sleep(SLEEP_AFTER_FAILS.totalMilliseconds());
			}

			fails++;
			Poco::Thread::sleep(SLEEP_BETWEEN_FAILS.totalMilliseconds());
		}
	}

	for (auto &node : m_zwaveNodes) {
		DeviceID deviceID = buildID(node.first);
		bool paired = deviceIDs.find(deviceID) != deviceIDs.end();

		try {
			createDevice(deviceID, paired);
		}
		catch (const Poco::ExistsException &ex) {
			logger().log(ex, __FILE__, __LINE__);
		}
	}
}

void ZWaveDeviceManager::createDevice(const DeviceID &deviceID, bool paired)
{
	auto it = m_zwaveNodes.find(nodeIDFromDeviceID(deviceID));
	if (it == m_zwaveNodes.end())
		return;

	ZWaveNodeInfo device = ZWaveNodeInfo::build(m_homeID, it->first);
	device.setDeviceID(deviceID);
	device.setPaired(paired);

	ZWaveDeviceInfo::Ptr msg =
		m_registry->find(device.vendorID(), device.productID());

	for (auto &valueID : it->second) {
		bool contains =
			msg->registry()->contains(valueID.GetCommandClassId(), valueID.GetIndex());

		logger().trace(
			"node ID " + to_string(it->first)
			+ " has "
			+ (contains ? "supported" : "unsupported")
			+ " CommandClass: "
			+ to_string(valueID.GetCommandClassId())
			+ " and index: "
			+ to_string(valueID.GetIndex()),
			__FILE__, __LINE__
		);

		if (!contains)
			continue;

		configureDefaultUnit(valueID);

		device.addValueID(
			valueID,
			msg->registry()->find(valueID.GetCommandClassId(), valueID.GetIndex())
		);
	}

	m_beeeonDevices.emplace(it->first, device);
	m_zwaveNodes.erase(it);
}

void ZWaveDeviceManager::configureDefaultUnit(OpenZWave::ValueID &valueID)
{
	const string unit = Manager::Get()->GetValueUnits(valueID);
	if (unit == "F" || unit == "C" )
		Manager::Get()->SetValueUnits(valueID, "C");
}

uint8_t ZWaveDeviceManager::nodeIDFromDeviceID(
		const DeviceID &deviceID) const
{
	return deviceID.ident() & NODE_ID_MASK;
}

bool ZWaveDeviceManager::modifyValue(uint8_t nodeID,
	const ModuleID &moduleID, const string &value)
{
	auto it = m_beeeonDevices.find(nodeID);
	if (it == m_beeeonDevices.end()) {
		logger().warning(
			"unknown node ID: "
			+ to_string(nodeID),
			__FILE__, __LINE__
		);

		return false;
	}

	ValueID valueID = it->second.findValueID(moduleID.value());
	ValueID::ValueType valueType = valueID.GetType();

	double data;
	switch(valueType) {
	case ValueID::ValueType_Bool:
		data = Poco::NumberParser::parseFloat(value);
		Manager::Get()->SetValue(valueID, static_cast<bool>(data));
		break;
	case ValueID::ValueType_Byte:
		data = Poco::NumberParser::parseFloat(value);
		Manager::Get()->SetValue(valueID, static_cast<uint8_t>(data));
		break;
	case ValueID::ValueType_Short:
		data = Poco::NumberParser::parseFloat(value);
		Manager::Get()->SetValue(valueID, static_cast<int16_t>(data));
		break;
	case ValueID::ValueType_Int:
		data = Poco::NumberParser::parseFloat(value);
		Manager::Get()->SetValue(valueID, static_cast<int>(data));
		break;
	case ValueID::ValueType_List:
		Manager::Get()->SetValueListSelection(valueID, value);
		break;
	default:
		logger().error(
			"unsupported ValueID Type: "
			+ to_string(valueType),
			__FILE__, __LINE__);
		return false;
	}

	return true;
}

DeviceID ZWaveDeviceManager::buildID(uint8_t nodeID) const
{
	uint64_t deviceID = 0;
	uint64_t homeId64 = m_homeID;

	deviceID |= homeId64 << 8;
	deviceID |= nodeID;

	return DeviceID(DevicePrefix::PREFIX_ZWAVE, deviceID);
}

void ZWaveDeviceManager::setUserPath(const string &userPath)
{
	m_userPath = userPath;
}

void ZWaveDeviceManager::setConfigPath(const string &configPath)
{
	m_configPath = configPath;
}

void ZWaveDeviceManager::setPollInterval(int pollInterval)
{
	m_pollInterval = pollInterval;
}

void ZWaveDeviceManager::setDeviceInfoRegistry(
	ZWaveDeviceInfoRegistry::Ptr factory)
{
	m_registry = factory;
}

void ZWaveDeviceManager::stop()
{
	DeviceManager::stop();
	m_stopEvent.set();
}
