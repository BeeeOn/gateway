#include <Poco/Clock.h>
#include <Poco/Exception.h>
#include <Poco/File.h>
#include <Poco/Logger.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/Thread.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <openzwave/Driver.h>
#include <openzwave/Manager.h>
#include <openzwave/Options.h>
#include <openzwave/command_classes/CommandClasses.h>
#include <openzwave/platform/Log.h>
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

#include "di/Injectable.h"
#include "hotplug/HotplugEvent.h"
#include "zwave/OZWNetwork.h"
#include "zwave/OZWPocoLoggerAdapter.h"
#include "zwave/ZWaveNodeEvent.h"
#include "zwave/ZWaveNotificationEvent.h"
#include "zwave/ZWaveDriverEvent.h"

BEEEON_OBJECT_BEGIN(BeeeOn, OZWNetwork)
BEEEON_OBJECT_CASTABLE(HotplugListener)
BEEEON_OBJECT_CASTABLE(ZWaveNetwork)
BEEEON_OBJECT_PROPERTY("configPath", &OZWNetwork::setConfigPath)
BEEEON_OBJECT_PROPERTY("userPath", &OZWNetwork::setUserPath)
BEEEON_OBJECT_PROPERTY("pollInterval", &OZWNetwork::setPollInterval)
BEEEON_OBJECT_PROPERTY("intervalBetweenPolls", &OZWNetwork::setIntervalBetweenPolls)
BEEEON_OBJECT_PROPERTY("retryTimeout", &OZWNetwork::setRetryTimeout)
BEEEON_OBJECT_PROPERTY("statisticsInterval", &OZWNetwork::setStatisticsInterval)
BEEEON_OBJECT_PROPERTY("networkKey", &OZWNetwork::setNetworkKey)
BEEEON_OBJECT_PROPERTY("controllersToReset", &OZWNetwork::setControllersToReset)
BEEEON_OBJECT_PROPERTY("executor", &OZWNetwork::setExecutor)
BEEEON_OBJECT_PROPERTY("listeners", &OZWNetwork::registerListener)
BEEEON_OBJECT_HOOK("done", &OZWNetwork::configure)
BEEEON_OBJECT_HOOK("cleanup", &OZWNetwork::cleanup)
BEEEON_OBJECT_END(BeeeOn, OZWNetwork)

using namespace std;
using namespace OpenZWave;
using namespace Poco;
using namespace BeeeOn;

OZWNetwork::OZWNode::OZWNode(const ZWaveNode::Identity &id, bool controller):
		ZWaveNode(id, controller)
{
}

void OZWNetwork::OZWNode::add(const CommandClass &cc, const OpenZWave::ValueID &id)
{
	ZWaveNode::add(cc);
	m_valueIDs.emplace(cc, id);
}

OpenZWave::ValueID OZWNetwork::OZWNode::operator[] (const CommandClass &cc) const
{
	auto it = m_valueIDs.find(cc);
	if (it == m_valueIDs.end())
		throw NotFoundException("command class " + cc.toString() + " not found");

	return it->second;
}

#define OZW_DEFAULT_POLL_INTERVAL          (0 * Timespan::SECONDS)
#define OZW_DEFAULT_INTERVAL_BETWEEN_POLLS false
#define OZW_DEFAULT_RETRY_TIMEOUT          (10 * Timespan::SECONDS)
#define OZW_DEFAULT_ASSUME_AWAKE           false
#define OZW_DEFAULT_DRIVER_MAX_ATTEMPTS    0

OZWNetwork::OZWNetwork():
	m_configPath("/etc/openzwave"),
	m_userPath("/var/cache/beeeon/openzwave"),
	m_pollInterval(OZW_DEFAULT_POLL_INTERVAL),
	m_intervalBetweenPolls(OZW_DEFAULT_INTERVAL_BETWEEN_POLLS),
	m_retryTimeout(OZW_DEFAULT_RETRY_TIMEOUT),
	m_assumeAwake(OZW_DEFAULT_ASSUME_AWAKE),
	m_driverMaxAttempts(OZW_DEFAULT_DRIVER_MAX_ATTEMPTS),
	m_configured(false),
	m_command(*this)
{
}

OZWNetwork::~OZWNetwork()
{
}

void OZWNetwork::setConfigPath(const string &path)
{
	m_configPath = path;
	m_configPath.makeDirectory();
}

void OZWNetwork::setUserPath(const string &path)
{
	m_userPath = path;
	m_userPath.makeDirectory();
}

void OZWNetwork::setPollInterval(const Timespan &interval)
{
	if (interval != 0 && interval < 1 * Timespan::SECONDS) {
		throw Poco::InvalidArgumentException(
			"pollInterval must be at least 1 s or 0");
	}

	m_pollInterval = interval;
}

void OZWNetwork::setIntervalBetweenPolls(bool enable)
{
	m_intervalBetweenPolls = enable;
}

void OZWNetwork::setRetryTimeout(const Timespan &timeout)
{
	if (timeout < 1 * Timespan::SECONDS)
		throw InvalidArgumentException("retryTimeout must be at least 1 s");

	m_retryTimeout = timeout;
}

void OZWNetwork::setAssumeAwake(bool awake)
{
	m_assumeAwake = awake;
}

void OZWNetwork::setDriverMaxAttempts(int attempts)
{
	if (attempts < 0)
		throw InvalidArgumentException("driverMaxAttempts must be non-negative");

	m_driverMaxAttempts = attempts;
}

void OZWNetwork::setNetworkKey(const list<string> &bytes)
{
	if (!bytes.empty() && bytes.size() != 16) {
		throw InvalidArgumentException(
			"networkKey must be either empty or 16 bytes long");
	}

	m_networkKey.clear();

	for (const auto &byte : bytes) {
		unsigned int octet = NumberParser::parseHex(byte);
		if (octet > 0x0ff) {
			throw InvalidArgumentException(
				"networkKey must only consist of values 0x00..0xFF");
		}

		m_networkKey.emplace_back((uint8_t) octet);
	}
}

void OZWNetwork::setStatisticsInterval(const Timespan &interval)
{
	if (interval <= 0) {
		throw InvalidArgumentException(
			"statistics interval must be a positive number");
	}

	m_statisticsRunner.setInterval(interval);
}

void OZWNetwork::setControllersToReset(const list<string> &homes)
{
	for (const auto &home : homes)
		m_controllersToReset.emplace((uint32_t) NumberParser::parseHex(home));
}

void OZWNetwork::registerListener(ZWaveListener::Ptr listener)
{
	m_eventSource.addListener(listener);
}

void OZWNetwork::setExecutor(AsyncExecutor::Ptr executor)
{
	m_executor = executor;
	m_eventSource.setAsyncExecutor(executor);
}

void OZWNetwork::checkDirectory(const Path &path)
{
	File file(path);

	if (!file.exists()) {
		logger().warning("no such directory " + file.path(),
				__FILE__, __LINE__);
	}
	else if (!file.canRead()) {
		throw FileAccessDeniedException("cannot read from " + file.path());
	}
}

void OZWNetwork::prepareDirectory(const Path &path)
{
	File file(path);

	if (!file.exists()) {
		logger().notice("creating directory " + file.path(),
				__FILE__, __LINE__);
		file.createDirectories();
	}
	else if (!file.canWrite()) {
		throw FileReadOnlyException("cannot write into " + file.path());
	}
	else if (!file.canRead()) {
		throw FileAccessDeniedException("cannot read from " + file.path());
	}
}

void OZWNetwork::configure()
{
	FastMutex::ScopedLock guard0(m_lock);
	FastMutex::ScopedLock guard1(m_managerLock);

	checkDirectory(m_configPath);
	prepareDirectory(m_userPath);

	Options::Create(m_configPath.toString(), m_userPath.toString(), "");

	Options::Get()->AddOptionInt("PollInterval",
			m_pollInterval.totalMilliseconds());
	Options::Get()->AddOptionBool("IntervalBetweenPolls",
			m_intervalBetweenPolls),
	Options::Get()->AddOptionInt("RetryTimeout",
			m_retryTimeout.totalMilliseconds());
	Options::Get()->AddOptionBool("AssumeAwake",
			m_assumeAwake);
	Options::Get()->AddOptionInt("DriverMaxAttempts",
			m_driverMaxAttempts);

	Options::Get()->AddOptionBool("Logging", true);
	Options::Get()->AddOptionBool("AppendLogFile", false);
	Options::Get()->AddOptionBool("ConsoleOutput", false);
	Options::Get()->AddOptionBool("SaveConfiguration", false);

	Logger &ozwLogger = Logger::get("OpenZWaveLibrary");

	Options::Get()->AddOptionInt("SaveLogLevel",
		OZWPocoLoggerAdapter::fromPocoLevel(ozwLogger.getLevel()));
	Options::Get()->AddOptionInt("QueueLogLevel",
		OZWPocoLoggerAdapter::fromPocoLevel(ozwLogger.getLevel()));

	if (!m_networkKey.empty()) {
		string key;

		for (const auto &byte : m_networkKey) {
			if (!key.empty())
				key += ",";

			key += NumberFormatter::formatHex(byte, 2, true);
		}

		Options::Get()->AddOptionString("NetworkKey", key, false);
	}

	Options::Get()->Lock();

	Manager::Create();

	// logger is deleted by OpenZWave library
	Log::SetLoggingClass(new OZWPocoLoggerAdapter(ozwLogger));

	m_statisticsRunner.start([&]() {
		fireStatistics();
	});

	Manager::Get()->AddWatcher(&ozwNotification, this);
	m_configured = true;
}

void OZWNetwork::cleanup()
{
	if (!m_configured)
		return;

	FastMutex::ScopedLock guard0(m_lock);
	FastMutex::ScopedLock guard1(m_managerLock);

	Manager::Get()->RemoveWatcher(&ozwNotification, this);

	try {
		OZWCommand::ScopedLock guard(m_command);
		m_command.cancelIf(m_command.type(), 200 * Timespan::MILLISECONDS);
	}
	catch (const IllegalStateException &e) {
		logger().warning(e.displayText(), __FILE__, __LINE__);
	}
	BEEEON_CATCH_CHAIN(logger())

	m_statisticsRunner.stop();
	Manager::Destroy();
	Options::Destroy();
}

bool OZWNetwork::matchEvent(const HotplugEvent &event)
{
	if (!event.properties()->has("tty.BEEEON_DONGLE"))
		return false;

	return "zwave" == event.properties()->getString("tty.BEEEON_DONGLE");
}

void OZWNetwork::onAdd(const HotplugEvent &event)
{
	if (!matchEvent(event))
		return;

	logger().information("registering dongle " + event.toString());

	Driver::ControllerInterface iftype = Driver::ControllerInterface_Unknown;

	if (event.subsystem() == "tty")
		iftype = Driver::ControllerInterface_Serial;
	else
		iftype = Driver::ControllerInterface_Hid;

	FastMutex::ScopedLock guard(m_managerLock);
	Manager::Get()->AddDriver(event.node(), iftype);
}

void OZWNetwork::onRemove(const HotplugEvent &event)
{
	if (!matchEvent(event))
		return;

	FastMutex::ScopedLock guard(m_managerLock);
	Manager::Get()->RemoveDriver(event.node());

	logger().information("dongle unregistered " + event.toString());
}

void OZWNetwork::ozwNotification(
	Notification const *notification, void *context)
{
	OZWNetwork *processor =
		reinterpret_cast<OZWNetwork *>(context);

	try {
		processor->onNotification(notification);
	}
	BEEEON_CATCH_CHAIN(Loggable::forInstance(processor))
}

bool OZWNetwork::ignoreNotification(const Notification *n) const
{
	const auto id = n->GetValueID();

	switch (n->GetType()) {
	case Notification::Type_ValueAdded:
	case Notification::Type_ValueChanged:
	case Notification::Type_ValueRefreshed:
		/*
		 * OZW adds 3 special Alarm types that does not exist
		 * in Z-Wave. Avoid processing of those.
		 */
		if (id.GetCommandClassId() == 0x71) {
			if (id.GetIndex() < 3)
				return true;
		}
		break;

	default:
		break;
	}

	return false;
}

void OZWNetwork::onNotification(const Notification *n)
{
	ZWaveNotificationEvent e(*n);
	m_eventSource.fireEvent(e, &ZWaveListener::onNotification);

	if (ignoreNotification(n)) {
		logger().debug("ignored notification " + n->GetAsString(),
				__FILE__, __LINE__);
		return;
	}

	const auto type = n->GetType();

	if (logger().trace()) {
		logger().trace(
			"start handling notification: " + to_string(type),
			__FILE__, __LINE__);
	}

	FastMutex::ScopedLock guard(m_lock);

	if (logger().trace()) {
		logger().trace(
			"handling notification: " + to_string(type),
			__FILE__, __LINE__);
	}

	switch (type) {
	case Notification::Type_DriverReady:
		driverReady(n);
		break;

	case Notification::Type_DriverFailed:
		driverFailed(n);
		break;

	case Notification::Type_DriverRemoved:
		driverRemoved(n);
		break;

	case Notification::Type_NodeNew:
		nodeNew(n);
		break;

	case Notification::Type_NodeAdded:
		nodeAdded(n);
		break;

	case Notification::Type_NodeNaming:
		nodeNaming(n);
		break;

	case Notification::Type_NodeProtocolInfo:
		nodeProtocolInfo(n);
		break;

	case Notification::Type_EssentialNodeQueriesComplete:
		nodeReady(n);
		break;

	case Notification::Type_NodeRemoved:
	case Notification::Type_NodeReset:
		nodeRemoved(n);
		break;

	case Notification::Type_ValueAdded:
		valueAdded(n);
		break;

	case Notification::Type_ValueChanged:
	case Notification::Type_ValueRefreshed:
		valueChanged(n);
		break;

	case Notification::Type_NodeQueriesComplete:
		nodeQueried(n);
		break;

	case Notification::Type_AwakeNodesQueried:
		awakeNodesQueried(n);
		break;

	case Notification::Type_AllNodesQueriedSomeDead:
	case Notification::Type_AllNodesQueried:
		allNodesQueried(n);
		break;

	default:
		break;
	}

	if (logger().trace()) {
		logger().trace(
			"finished handling notification: " + to_string(type),
			__FILE__, __LINE__);
	}
}

void OZWNetwork::resetController(const uint32_t home)
{
	logger().notice("resetting controller of home " + homeAsString(home),
			__FILE__, __LINE__);
	auto &lock = m_managerLock;

	m_executor->invoke([home, &lock]() {
		FastMutex::ScopedLock guard(const_cast<FastMutex&>(lock));
		Manager::Get()->ResetController(home);
	});
}

void OZWNetwork::driverReady(const Notification *n)
{
	map<uint8_t, OZWNode> empty;
	auto result = m_homes.emplace(n->GetHomeId(), empty);
	if (!result.second)
		return;

	logger().notice("new home " + homeAsString(n->GetHomeId()),
			__FILE__, __LINE__);

	auto shouldReset = m_controllersToReset.find(n->GetHomeId());
	if (shouldReset != m_controllersToReset.end()) {
		m_controllersToReset.erase(shouldReset);
		resetController(n->GetHomeId());
	}
	else {
		FastMutex::ScopedLock guard(m_managerLock);
		const auto home = n->GetHomeId();

		logger().information(
			"home " + homeAsString(home)
			+ " Z-Wave: "
			+ Manager::Get()->GetLibraryTypeName(home)
			+ " "
			+ Manager::Get()->GetLibraryVersion(home),
			__FILE__, __LINE__);

		Manager::Get()->WriteConfig(n->GetHomeId());
	}
}

void OZWNetwork::driverFailed(const Notification *n)
{
	auto it = m_homes.find(n->GetHomeId());
	if (it != m_homes.end())
		m_homes.erase(it);

	logger().critical("failed to initialize driver for home "
			+ homeAsString(n->GetHomeId()),
			__FILE__, __LINE__);
}

void OZWNetwork::driverRemoved(const Notification *n)
{
	auto it = m_homes.find(n->GetHomeId());
	if (it == m_homes.end())
		return;

	m_homes.erase(it);

	logger().notice("removed home " + homeAsString(n->GetHomeId()),
			__FILE__, __LINE__);
}

bool OZWNetwork::checkNodeIsController(const uint32_t home, const uint8_t node) const
{
	FastMutex::ScopedLock guard(m_managerLock);
	return Manager::Get()->GetControllerNodeId(home) == node;
}

void OZWNetwork::nodeNew(const Notification *n)
{
	const bool controller = checkNodeIsController(n->GetHomeId(), n->GetNodeId());
	ZWaveNode node({n->GetHomeId(), n->GetNodeId()}, controller);

	if (logger().debug()) {
		logger().debug("discovered new node: "
				+ node.toString(),
				__FILE__, __LINE__);
	}
}

void OZWNetwork::nodeAdded(const Notification *n)
{
	auto home = m_homes.find(n->GetHomeId());
	if (home == m_homes.end())
		return;

	const bool controller = checkNodeIsController(n->GetHomeId(), n->GetNodeId());

	OZWNode node({n->GetHomeId(), n->GetNodeId()}, controller);

	auto result = home->second.emplace(n->GetNodeId(), node);
	if (!result.second)
		return;

	if (logger().debug()) {
		logger().debug("node added to Z-Wave network: "
				+ node.toString(),
				__FILE__, __LINE__);
	}
}

void OZWNetwork::nodeNaming(const Notification *n)
{
	auto home = m_homes.find(n->GetHomeId());
	if (home == m_homes.end())
		return;

	auto it = home->second.find(n->GetNodeId());
	if (it == home->second.end())
		return;

	ZWaveNode &node = it->second;
	const auto &id = node.id();

	FastMutex::ScopedLock guard(m_managerLock);

	const auto productId = Manager::Get()
		->GetNodeProductId(id.home, id.node);
	const auto product = Manager::Get()
		->GetNodeProductName(id.home, id.node);

	const auto productType = Manager::Get()
		->GetNodeProductType(id.home, id.node);

	const auto vendorId = Manager::Get()
		->GetNodeManufacturerId(id.home, id.node);
	const auto vendor = Manager::Get()
		->GetNodeManufacturerName(id.home, id.node);

	const auto name = Manager::Get()->GetNodeName(id.home, id.node);

	node.setProductId(NumberParser::parseHex(productId));
	node.setProduct(product);
	node.setProductType(NumberParser::parseHex(productType));
	node.setVendorId(NumberParser::parseHex(vendorId));
	node.setVendor(vendor);

	logger().information(
		"resolved node " + node.toString() + " identification: "
		+ node.toInfoString() + " '" + name + "'",
		__FILE__, __LINE__);

	notifyEvent(PollEvent::createNewNode(node));
}

void OZWNetwork::nodeProtocolInfo(const Notification *n)
{
	auto home = m_homes.find(n->GetHomeId());
	if (home == m_homes.end())
		return;

	auto it = home->second.find(n->GetNodeId());
	if (it == home->second.end())
		return;

	ZWaveNode &node = it->second;
	uint32_t support = 0;

	FastMutex::ScopedLock guard(m_managerLock);

	if (Manager::Get()->IsNodeZWavePlus(n->GetHomeId(), n->GetNodeId())) {
		support |= ZWaveNode::SUPPORT_ZWAVEPLUS;

		if (logger().debug()) {
			logger().debug("node " + node.toString()
				+ " is ZWavePlus device: "
				+ Manager::Get()->GetNodePlusTypeString(
						n->GetHomeId(), n->GetNodeId())
					+ ", "
				+ Manager::Get()->GetNodeRoleString(
						n->GetHomeId(), n->GetNodeId()),
				__FILE__, __LINE__);
		}
	}

	if (Manager::Get()->IsNodeListeningDevice(n->GetHomeId(), n->GetNodeId()))
		support |= ZWaveNode::SUPPORT_LISTENING;
	if (Manager::Get()->IsNodeBeamingDevice(n->GetHomeId(), n->GetNodeId()))
		support |= ZWaveNode::SUPPORT_BEAMING;
	if (Manager::Get()->IsNodeRoutingDevice(n->GetHomeId(), n->GetNodeId()))
		support |= ZWaveNode::SUPPORT_ROUTING;
	if (Manager::Get()->IsNodeSecurityDevice(n->GetHomeId(), n->GetNodeId()))
		support |= ZWaveNode::SUPPORT_SECURITY;

	node.setSupport(support);
}

void OZWNetwork::nodeReady(const Notification *n)
{
	auto home = m_homes.find(n->GetHomeId());
	if (home == m_homes.end())
		return;

	auto it = home->second.find(n->GetNodeId());
	if (it == home->second.end())
		return;

	const ZWaveNode &node = it->second;

	logger().information(
		"node " + node.toString()
		+ " is ready to work",
		__FILE__, __LINE__);
}

void OZWNetwork::nodeRemoved(const Notification *n)
{
	auto home = m_homes.find(n->GetHomeId());
	if (home == m_homes.end())
		return;

	auto it = home->second.find(n->GetNodeId());
	if (it == home->second.end())
		return;

	if (logger().debug()) {
		logger().debug(
			"node " + it->second.toString() + " removed",
			__FILE__, __LINE__);
	}

	notifyEvent(PollEvent::createRemoveNode(it->second));
	home->second.erase(it);

	FastMutex::ScopedLock guard(m_managerLock);
	Manager::Get()->WriteConfig(n->GetHomeId());
}

void OZWNetwork::valueAdded(const Notification *n)
{
	auto home = m_homes.find(n->GetHomeId());
	if (home == m_homes.end())
		return;

	auto it = home->second.find(n->GetNodeId());
	if (it == home->second.end())
		return;

	const ZWaveNode::CommandClass &cc =
		buildCommandClass(n->GetValueID());

	if (logger().trace()) {
		logger().trace("discovered new value " + cc.toString()
				+ " for node " + it->second.toString(),
				__FILE__, __LINE__);
	}

	it->second.add(cc, n->GetValueID());
}

void OZWNetwork::valueChanged(const Notification *n)
{
	auto home = m_homes.find(n->GetHomeId());
	if (home == m_homes.end())
		return;

	auto it = home->second.find(n->GetNodeId());
	if (it == home->second.end())
		return;

	FastMutex::ScopedLock guard(m_managerLock);

	string value;
	Manager::Get()->GetValueAsString(n->GetValueID(), &value);

	const string &unit = Manager::Get()->GetValueUnits(n->GetValueID());
	const auto cc = buildCommandClass(n->GetValueID());

	if (logger().debug()) {
		logger().debug("received data " + value
				+ " (" + cc.toString() + ") from "
				+ it->second.toString(),
				__FILE__, __LINE__);
	}

	notifyEvent(PollEvent::createValue({
		it->second,
		cc,
		value,
		unit
	}));
}

void OZWNetwork::nodeQueried(const Notification *n)
{
	auto home = m_homes.find(n->GetHomeId());
	if (home == m_homes.end())
		return;

	auto it = home->second.find(n->GetNodeId());
	if (it == home->second.end())
		return;

	it->second.setQueried(true);

	notifyEvent(PollEvent::createUpdateNode(it->second));

	FastMutex::ScopedLock guard(m_managerLock);
	Manager::Get()->WriteConfig(n->GetHomeId());
}

void OZWNetwork::awakeNodesQueried(const Notification *n)
{
	const auto home = m_homes.find(n->GetHomeId());
	if (home == m_homes.end())
		return;

	size_t queried = 0;
	size_t failed = 0; // according to OZW
	size_t sleeping = 0;
	size_t total = 0;

	for (auto &pair : home->second) {
		ZWaveNode &node = pair.second;
		const auto &id = node.id();

		FastMutex::ScopedLock guard(m_managerLock);

		if (Manager::Get()->IsNodeFailed(id.home, id.node)) {
			failed += 1;
		}
		else {
			if (Manager::Get()->IsNodeAwake(id.home, id.node)) {
				if (!node.queried()) {
					node.setQueried(true);
					notifyEvent(PollEvent::createUpdateNode(node));
				}
			}
			else {
				sleeping += 1;

				logger().debug("node " + node.toString()
						+ " is sleeping",
						__FILE__, __LINE__);
			}
		}

		if (node.queried())
			queried += 1;

		total += 1;
	}

	if (logger().debug()) {
		logger().debug("awaken nodes for home "
				+ homeAsString(n->GetHomeId())
				+ " queried ("
				+ to_string(queried)
				+ "/"
				+ to_string(failed)
				+ "/"
				+ to_string(sleeping)
				+ "/"
				+ to_string(total) + ")",
				__FILE__, __LINE__);
	}

	notifyEvent(PollEvent::createReady());
}

void OZWNetwork::allNodesQueried(const Notification *n)
{
	const auto home = m_homes.find(n->GetHomeId());
	if (home == m_homes.end())
		return;

	size_t failed = 0; // according to OZW
	size_t total = 0;

	for (auto &pair : home->second) {
		ZWaveNode &node = pair.second;
		const auto &id = node.id();

		FastMutex::ScopedLock guard(m_managerLock);

		if (Manager::Get()->IsNodeFailed(id.home, id.node)) {
			failed += 1;
		}
		else if (!node.queried()) {
			node.setQueried(true);
			notifyEvent(PollEvent::createUpdateNode(node));
		}

		total += 1;
	}

	if (logger().debug()) {
		logger().debug("all nodes for home "
				+ homeAsString(n->GetHomeId())
				+ " queried ("
				+ to_string(failed) + "/"
				+ to_string(total) + ")",
				__FILE__, __LINE__);
	}

	notifyEvent(PollEvent::createReady());
}

string OZWNetwork::homeAsString(uint32_t home)
{
	return NumberFormatter::formatHex(home, 8);
}

ZWaveNode::CommandClass OZWNetwork::buildCommandClass(
		const OpenZWave::ValueID &id)
{
	const uint8_t cc = id.GetCommandClassId();
	uint8_t index = id.GetIndex();

	if (cc == 0x71) { // fixup Alarm fucked up by OZW
		poco_assert_msg(index >= 3, "Alarm index < 3");
		index -= 3;
	}

	return {
		cc,
		index,
		id.GetInstance(),
		CommandClasses::GetName(cc)
	};
}

void OZWNetwork::startInclusion()
{
	FastMutex::ScopedLock guard(m_managerLock);

	for (const auto &home : m_homes) {
		if (!Manager::Get()->IsPrimaryController(home.first))
			continue;

		m_command.request(OZWCommand::INCLUSION, home.first);
		break;
	}
}

void OZWNetwork::cancelInclusion()
{
	FastMutex::ScopedLock guard(m_managerLock);

	if (m_command.cancelIf(OZWCommand::INCLUSION, 200 * Timespan::MILLISECONDS)) {
		if (logger().debug()) {
			logger().debug("command inclusion is being cancelled",
				__FILE__, __LINE__);
		}
	}
	else {
		if (logger().warning()) {
			logger().warning("command inclusion is not running,"
					" cancel was ignored",
				__FILE__, __LINE__);
		}
	}
}

void OZWNetwork::startRemoveNode()
{
	FastMutex::ScopedLock guard(m_managerLock);

	for (const auto &home : m_homes) {
		if (!Manager::Get()->IsPrimaryController(home.first))
			continue;

		m_command.request(OZWCommand::REMOVE_NODE, home.first);
		break;
	}
}

void OZWNetwork::cancelRemoveNode()
{
	FastMutex::ScopedLock guard(m_managerLock);

	if (m_command.cancelIf(OZWCommand::REMOVE_NODE, 200 * Timespan::MILLISECONDS)) {
		if (logger().debug()) {
			logger().debug("command remove-node is being cancelled",
				__FILE__, __LINE__);
		}
	}
	else {
		if (logger().warning()) {
			logger().warning("command remove-node is not running,"
					" cancel was ignored",
				__FILE__, __LINE__);
		}
	}
}

void OZWNetwork::interrupt()
{
	try {
		OZWCommand::ScopedLock guard(m_command);
		m_command.cancelIf(m_command.type(), 200 * Timespan::MILLISECONDS);
	}
	catch (const IllegalStateException &e) {
		logger().warning(e.displayText(), __FILE__, __LINE__);
	}
	BEEEON_CATCH_CHAIN(logger())

	AbstractZWaveNetwork::interrupt();
}

void OZWNetwork::fireStatistics()
{
	FastMutex::ScopedLock guard(m_managerLock);

	for (const auto &home : m_homes) {
		Driver::DriverData data;
		Manager::Get()->GetDriverStatistics(home.first, &data);

		ZWaveDriverEvent e(data);
		m_eventSource.fireEvent(e, &ZWaveListener::onDriverStats);

		for (const auto &node : home.second) {
			Node::NodeData data;
			Manager::Get()->GetNodeStatistics(home.first, node.first, &data);

			ZWaveNodeEvent e(data, node.first);
			m_eventSource.fireEvent(e, &ZWaveListener::onNodeStats);
		}
	}
}
