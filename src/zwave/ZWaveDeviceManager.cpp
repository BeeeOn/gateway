#include <list>

#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <openzwave/Manager.h>
#include <openzwave/Options.h>
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "hotplug/HotplugEvent.h"
#include "model/SensorData.h"
#include "zwave/GenericZWaveDeviceInfoRegistry.h"
#include "zwave/ZWaveDriverEvent.h"
#include "zwave/ZWaveDeviceManager.h"
#include "zwave/ZWaveNodeEvent.h"
#include "zwave/ZWaveNotificationEvent.h"
#include "zwave/ZWavePocoLoggerAdapter.h"
#include "zwave/ZWaveUtil.h"

BEEEON_OBJECT_BEGIN(BeeeOn, ZWaveDeviceManager)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(HotplugListener)
BEEEON_OBJECT_TEXT("userPath", &ZWaveDeviceManager::setUserPath)
BEEEON_OBJECT_TEXT("configPath", &ZWaveDeviceManager::setConfigPath)
BEEEON_OBJECT_TIME("pollInterval", &ZWaveDeviceManager::setPollInterval)
BEEEON_OBJECT_TIME("statisticsInterval", &ZWaveDeviceManager::setStatisticsInterval)
BEEEON_OBJECT_REF("commandDispatcher", &ZWaveDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_REF("distributor", &ZWaveDeviceManager::setDistributor)
BEEEON_OBJECT_REF("deviceInfoRegistry", &ZWaveDeviceManager::setDeviceInfoRegistry)
BEEEON_OBJECT_REF("listeners", &ZWaveDeviceManager::registerListener)
BEEEON_OBJECT_REF("executor", &ZWaveDeviceManager::setExecutor)
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
using Poco::Timespan;

static const int NODE_ID_MASK = 0xff;

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
	m_pollInterval(1 * Timespan::SECONDS),
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
	Options::Get()->AddOptionInt("PollInterval", m_pollInterval.totalMilliseconds());
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

	if (m_eventSource.asyncExecutor().isNull()) {
		logger().critical(
			"runtime statistics could not be send, executor was not set",
			__FILE__, __LINE__);
	}
	else {
		m_statisticsRunner.start([&]() {
			fireStatistics();
		});
	}
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

	m_statisticsRunner.stop();

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
		+ cmd->deviceID().toString(),
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
}

void ZWaveDeviceManager::stopCommand(Timer &)
{
	Manager::Get()->CancelControllerCommand(m_homeID);
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
			+ ZWaveUtil::buildID(m_homeID, nodeID).toString()
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
			+ ZWaveUtil::commandClass(
				valueID.GetCommandClassId(),
				valueID.GetIndex()),
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

void ZWaveDeviceManager::dispatchUnpairedDevices()
{
	if (m_state != State::LISTENING) {
		logger().warning(
			"attempt to send new devices out of listening mode",
			__FILE__, __LINE__);

		return;
	}

	for (auto &nodeID : m_beeeonDevices) {
		if (nodeID.second.paired())
			continue;

		if (nodeID.second.vendorName().empty() && nodeID.second.productName().empty()) {
			logger().trace(
				"no info about node id " + to_string(nodeID.first),
				__FILE__, __LINE__
			);
			continue;
		}

		dispatch(
			new NewDeviceCommand(
				ZWaveUtil::buildID(m_homeID, nodeID.first),
				nodeID.second.vendorName(),
				nodeID.second.productName(),
				nodeID.second.moduleTypes()
			)
		);
	}
}

void ZWaveDeviceManager::handleUnpairing(const Notification *notification)
{
	switch (notification->GetEvent()) {
	case OpenZWave::Driver::ControllerState_Starting:
		logger().information("starting unpairing process",
			__FILE__, __LINE__);
		break;

	case OpenZWave::Driver::ControllerState_Waiting:
		logger().information("expecting user actions",
			__FILE__, __LINE__);
		break;

	case OpenZWave::Driver::ControllerState_Sleeping:
		logger().debug("sleeping while waiting to unpair",
			__FILE__, __LINE__);
		break;

	case OpenZWave::Driver::ControllerState_InProgress:
		logger().debug("communicating with a device",
			__FILE__, __LINE__);
		break;

	case OpenZWave::Driver::ControllerState_Cancel:
		logger().information("cancelling unpairing process",
			__FILE__, __LINE__);
		break;

	case OpenZWave::Driver::ControllerState_Completed:
		logger().information("unpairing process completed",
			__FILE__, __LINE__);

		m_state = NODE_QUERIED;
		break;

	case OpenZWave::Driver::ControllerState_Error:
	case OpenZWave::Driver::ControllerState_Failed:
		logger().error("unpairing process has failed: "
			+ to_string(notification->GetNotification()),
			__FILE__, __LINE__);

		m_state = NODE_QUERIED;
		break; // ignore

	default:
		break;
	}
}

void ZWaveDeviceManager::handleListening(const Notification *notification)
{
	switch (notification->GetEvent()) {
	case OpenZWave::Driver::ControllerState_Starting:
		logger().information("starting discovery process",
			__FILE__, __LINE__);
		break;

	case OpenZWave::Driver::ControllerState_Waiting:
		logger().information("expecting user actions",
			__FILE__, __LINE__);
		break;

	case OpenZWave::Driver::ControllerState_Sleeping:
		logger().debug("sleeping while waiting for devices",
			__FILE__, __LINE__);
		break;

	case OpenZWave::Driver::ControllerState_InProgress:
		logger().debug("communicating with a device",
			__FILE__, __LINE__);
		break;

	case OpenZWave::Driver::ControllerState_Cancel:
		logger().information("cancelling discovery process",
			__FILE__, __LINE__);
		break;

	case OpenZWave::Driver::ControllerState_Completed:
		logger().information("discovery process completed",
			__FILE__, __LINE__);

		dispatchUnpairedDevices();
		m_state = NODE_QUERIED;
		break;

	case OpenZWave::Driver::ControllerState_Error:
	case OpenZWave::Driver::ControllerState_Failed:
		logger().error("discovery process has failed: "
			+ to_string(notification->GetNotification()),
			__FILE__, __LINE__);

		dispatchUnpairedDevices();
		m_state = NODE_QUERIED;
		break; // ignore

	default:
		break;
	}
}

void ZWaveDeviceManager::onNotification(
	const Notification *notification)
{
	ZWaveNotificationEvent e(*notification);
	m_eventSource.fireEvent(e, &ZWaveListener::onNotification);

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
	case Notification::Type_ControllerCommand:
		if (m_state == LISTENING)
			handleListening(notification);
		else if (m_state == UNPAIRING)
			handleUnpairing(notification);
				
		break;
	case Notification::Type_NodeQueriesComplete:
		createBeeeOnDevice(notification->GetNodeId());
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
		DeviceID deviceID = ZWaveUtil::buildID(m_homeID, node.first);
		bool paired = deviceIDs.find(deviceID) != deviceIDs.end();

		try {
			createDevice(node.first, node.second, paired);
		}
		catch (const Poco::ExistsException &ex) {
			logger().log(ex, __FILE__, __LINE__);
		}
	}

	m_zwaveNodes.clear();
}

void ZWaveDeviceManager::createDevice(
		const uint8_t nodeID,
		const std::list<OpenZWave::ValueID> &values,
		bool paired)
{
	ZWaveNodeInfo device = ZWaveNodeInfo::build(m_homeID, nodeID);
	device.setDeviceID(ZWaveUtil::buildID(m_homeID, nodeID));
	device.setPaired(paired);

	ZWaveDeviceInfo::Ptr msg =
		m_registry->find(device.vendorID(), device.productID());

	for (auto valueID : values) {
		bool contains =
			msg->registry()->contains(valueID.GetCommandClassId(), valueID.GetIndex());

		logger().information(
			"node ID " + to_string(nodeID)
			+ " has "
			+ (contains ? "supported" : "unsupported")
			+ " CommandClass: "
			+ ZWaveUtil::commandClass(
				valueID.GetCommandClassId(),
				valueID.GetIndex()),
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

	m_beeeonDevices.emplace(nodeID, device);
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

void ZWaveDeviceManager::fireStatistics()
{
	if (m_state == IDLE) {
		logger().debug("statistics cannot be sent, dongle is not ready");
		return;
	}

	Poco::FastMutex::ScopedLock guard(m_lock);
	fireDriverEventStatistics();

	for (auto &id : m_beeeonDevices)
		fireNodeEventStatistics(id.first);
}

void ZWaveDeviceManager::fireNodeEventStatistics(uint8_t nodeID)
{
	Node::NodeData data;
	Manager::Get()->GetNodeStatistics(m_homeID, nodeID, &data);
	ZWaveNodeEvent nodeEvent(data, nodeID);

	m_eventSource.fireEvent(nodeEvent, &ZWaveListener::onNodeStats);
}

void ZWaveDeviceManager::fireDriverEventStatistics()
{
	Driver::DriverData data;
	Manager::Get()->GetDriverStatistics(m_homeID, &data);
	ZWaveDriverEvent driverEvent(data);

	m_eventSource.fireEvent(driverEvent, &ZWaveListener::onDriverStats);
}

void ZWaveDeviceManager::setUserPath(const string &userPath)
{
	m_userPath = userPath;
}

void ZWaveDeviceManager::setConfigPath(const string &configPath)
{
	m_configPath = configPath;
}

void ZWaveDeviceManager::setPollInterval(const Timespan &pollInterval)
{
	if (pollInterval != 0 && pollInterval < 1 * Timespan::MILLISECONDS)
		throw Poco::InvalidArgumentException("pollInterval must be at least 1 ms or 0");

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
	answerQueue().dispose();
}

void ZWaveDeviceManager::createBeeeOnDevice(uint8_t nodeID)
{
	try {
		auto it = m_zwaveNodes.find(nodeID);
		if (it == m_zwaveNodes.end())
			return;

		createDevice(nodeID, it->second, false);
		m_zwaveNodes.erase(it);
	}
	catch (const Poco::ExistsException &ex) {
		logger().log(ex, __FILE__, __LINE__);
	}
}

void ZWaveDeviceManager::setStatisticsInterval(const Timespan &interval)
{
	if (interval <= 0) {
		throw Poco::InvalidArgumentException(
			"statistics interval must be a positive number");
	}

	m_statisticsRunner.setInterval(interval);
}

void ZWaveDeviceManager::registerListener(ZWaveListener::Ptr listener)
{
	m_eventSource.addListener(listener);
}

void ZWaveDeviceManager::setExecutor(Poco::SharedPtr<AsyncExecutor> executor)
{
	m_eventSource.setAsyncExecutor(executor);
}
