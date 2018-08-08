#include <Poco/NumberParser.h>
#include <Poco/Timespan.h>
#include <Poco/Timestamp.h>

#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "hotplug/HotplugEvent.h"
#include "iqrf/IQRFDeviceManager.h"
#include "iqrf/IQRFJsonResponse.h"
#include "iqrf/IQRFUtil.h"
#include "iqrf/request/DPACoordBondNodeRequest.h"
#include "iqrf/request/DPACoordBondedNodesRequest.h"
#include "iqrf/request/DPACoordClearAllBondsRequest.h"
#include "iqrf/request/DPACoordDiscoveryRequest.h"
#include "iqrf/request/DPACoordRemoveNodeRequest.h"
#include "iqrf/request/DPANodeRemoveBondRequest.h"
#include "iqrf/request/DPAOSBatchRequest.h"
#include "iqrf/request/DPAOSRestartRequest.h"
#include "iqrf/response/DPACoordBondNodeResponse.h"
#include "iqrf/response/DPACoordBondedNodesResponse.h"
#include "iqrf/response/DPACoordRemoveNodeResponse.h"
#include "util/BlockingAsyncWork.h"
#include "util/ClassInfo.h"
#include "util/DelayedAsyncWork.h"
#include "util/JsonUtil.h"
#include "util/MultiException.h"

BEEEON_OBJECT_BEGIN(BeeeOn, IQRFDeviceManager)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(HotplugListener)
BEEEON_OBJECT_CASTABLE(DeviceStatusHandler)
BEEEON_OBJECT_PROPERTY("receiveTimeout", &IQRFDeviceManager::setReceiveTimeout)
BEEEON_OBJECT_PROPERTY("refreshTime", &IQRFDeviceManager::setRefreshTime)
BEEEON_OBJECT_PROPERTY("refreshTimePeripheralInfo", &IQRFDeviceManager::setRefreshTimePeripheralInfo)
BEEEON_OBJECT_PROPERTY("distributor", &IQRFDeviceManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &IQRFDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("protocols", &IQRFDeviceManager::registerDPAProtocol)
BEEEON_OBJECT_PROPERTY("mqttConnector", &IQRFDeviceManager::setMqttConnector)
BEEEON_OBJECT_PROPERTY("devicesRetryTimeout", &IQRFDeviceManager::setIQRFDevicesRetryTimeout)
BEEEON_OBJECT_PROPERTY("coordinatorReset", &IQRFDeviceManager::setCoordinatorReset)
BEEEON_OBJECT_PROPERTY("devicePoller", &IQRFDeviceManager::setDevicePoller)
BEEEON_OBJECT_END(BeeeOn, IQRFDeviceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static Timespan IQRF_BONDING_TIME = 10 * Timespan::SECONDS;
static Timespan METHOD_TIMEOUT = 1 * Timespan::MINUTES;
static size_t THRESHOLD_WRITE_EXCEPTION = 5;

IQRFDeviceManager::IQRFDeviceManager():
	DongleDeviceManager(DevicePrefix::PREFIX_IQRF, {
		typeid(GatewayListenCommand),
		typeid(DeviceAcceptCommand),
		typeid(DeviceUnpairCommand),
	}),
	m_refreshTime(RefreshTime::fromSeconds(60)),
	m_refreshTimePeripheralInfo(RefreshTime::fromSeconds(300)),
	m_receiveTimeout(1 * Timespan::SECONDS),
	m_devicesRetryTimeout(300 * Timespan::SECONDS),
	m_bondingMode(false)
{
}

void IQRFDeviceManager::registerDPAProtocol(DPAProtocol::Ptr protocol)
{
	m_dpaProtocols.push_back(protocol);
}

void IQRFDeviceManager::setReceiveTimeout(
		const Timespan &timeout)
{
	if (timeout < 1 * Timespan::MILLISECONDS) {
		throw InvalidArgumentException(
			"receiveTimeout must be at least 1 ms");
	}

	m_receiveTimeout = timeout;
}

void IQRFDeviceManager::setRefreshTime(
		const Timespan &refresh)
{
	if (refresh < 1 * Timespan::SECONDS) {
		throw InvalidArgumentException(
			"refreshTime must be at least 1 s");
	}

	m_refreshTime = RefreshTime::fromSeconds(refresh.totalSeconds());
}

void IQRFDeviceManager::setRefreshTimePeripheralInfo(
		const Timespan &refresh)
{
	if (refresh < 1 * Timespan::SECONDS) {
		throw InvalidArgumentException(
			"refreshTimePeripheralInfo must be at least 1 s");
	}

	m_refreshTimePeripheralInfo = RefreshTime::fromSeconds(refresh.totalSeconds());
}

void IQRFDeviceManager::setIQRFDevicesRetryTimeout(const Timespan &timeout)
{
	if (timeout < 1 * Timespan::MILLISECONDS) {
		throw InvalidArgumentException(
			"daemonRetryTimeout must be at least 1 ms");
	}

	m_devicesRetryTimeout = timeout;
}

void IQRFDeviceManager::setCoordinatorReset(const string &reset)
{
	m_coordinatorReset = NumberParser::parseBool(reset);
}

void IQRFDeviceManager::setMqttConnector(IQRFMqttConnector::Ptr connector)
{
	m_connector = connector;
}

void IQRFDeviceManager::setDevicePoller(DevicePoller::Ptr poller)
{
	m_pollingKeeper.setDevicePoller(poller);
}

void IQRFDeviceManager::stop()
{
	answerQueue().dispose();
	DongleDeviceManager::stop();
}

string IQRFDeviceManager::dongleMatch(const HotplugEvent &e)
{
	if (!e.properties()->has("tty.BEEEON_DONGLE") && !e.properties()->has("spi.BEEEON_DONGLE"))
		return "";

	string dongle;
	if (e.properties()->has("tty.BEEEON_DONGLE"))
		dongle = e.properties()->getString("tty.BEEEON_DONGLE");
	else
		dongle = e.properties()->getString("spi.BEEEON_DONGLE");

	if (dongle != "iqrf")
		return "";

	return e.name();
}

void IQRFDeviceManager::dongleAvailable()
{
	logger().notice(
		"starting IQRF device manager",
		__FILE__, __LINE__);

	if (m_coordinatorReset)
		coordinatorResetProcess();

	logger().information(
		"supported protocols: " + supportedProtocolsToString(),
		__FILE__, __LINE__);

	while (!m_stopControl.shouldStop()) {
		if (!m_bondingMode) {
			try {
				syncBondedNodes(METHOD_TIMEOUT);
			}
			catch (const IllegalStateException &e) {
				logger().warning(
					"IQRF daemon looks like it is not running",
					__FILE__, __LINE__);
			}
		}

		m_stopControl.waitStoppable(m_devicesRetryTimeout);
	}

	logger().notice(
		"stopping IQRF device manager",
		__FILE__, __LINE__);
}

void IQRFDeviceManager::coordinatorResetProcess()
{
	logger().notice(
		"start of coordinator reset",
		__FILE__, __LINE__);

	IQRFUtil::makeRequest(
		m_connector,
		new DPACoordClearAllBondsRequest,
		m_receiveTimeout
	);

	logger().notice(
		"reset of coordinator was successful",
		__FILE__, __LINE__);

	m_coordinatorReset = false;
}

string IQRFDeviceManager::supportedProtocolsToString() const
{
	string repr;

	for (const auto &protocol : m_dpaProtocols) {
		if (!repr.empty())
			repr += ", ";

		repr += ClassInfo::forPointer(protocol.get()).name();
	}

	return repr;
}

void IQRFDeviceManager::syncBondedNodes(const Timespan &methodTimeout)
{
	FastMutex::ScopedLock guard(m_lock);
	const Clock started;

	auto bondedNodes = obtainBondedNodes(methodTimeout - started.elapsed());
	auto nonSynchronizedNodes = obtainNonSynchronizedNodes(bondedNodes);

	auto obtainedDevices = obtainDeviceInfo(
		methodTimeout - started.elapsed(), nonSynchronizedNodes);

	for (const auto &device : obtainedDevices) {
		if (deviceCache()->paired(device.first)) {
			m_devices.emplace(device.first, device.second);
			m_pollingKeeper.schedule(device.second);
		}
	}

	for (auto deviceIt = begin(m_devices); deviceIt != end(m_devices); ++deviceIt) {
		auto nodeIt = bondedNodes.find(deviceIt->second->networkAddress());
		if (nodeIt == bondedNodes.end()) {
			m_pollingKeeper.cancel(deviceIt->first);
			deviceIt = m_devices.erase(deviceIt);
		}
	}
}

map<DeviceID, IQRFDevice::Ptr> IQRFDeviceManager::obtainDeviceInfo(
	const Timespan &methodTimeout,
	const set<uint8_t> &nodes)
{
	const Clock started;
	map<DeviceID, IQRFDevice::Ptr> bonded;

	for (const auto &node : nodes) {
		try {
			IQRFDevice::Ptr dev =
				tryObtainDeviceInfo(node,
					methodTimeout - started.elapsed());

			if (logger().debug()) {
				logger().debug(
					"obtained device: "
					+ dev->toString(), __FILE__, __LINE__);
			}
			bonded.insert({dev->id(), dev});
		}
		BEEEON_CATCH_CHAIN(logger());
	}

	return bonded;
}

IQRFDevice::Ptr IQRFDeviceManager::tryObtainDeviceInfo(
	DPAMessage::NetworkAddress node,
	const Timespan &methodTimeout
)
{
	if (logger().debug()) {
		logger().debug(
			"obtaining device info for device: " + to_string(node),
			__FILE__, __LINE__);
	}

	DPAProtocol::Ptr protocol =
		detectNodeProtocol(node, methodTimeout);

	IQRFDevice::Ptr device =
		new IQRFDevice(m_connector,
			m_receiveTimeout,
			node,
			protocol,
			m_refreshTime,
			m_refreshTimePeripheralInfo);
	device->probe(methodTimeout);

	return device;
}

set<uint8_t> IQRFDeviceManager::obtainBondedNodes(
	const Timespan &methodTimeout)
{
	if (m_receiveTimeout > methodTimeout) {
		throw TimeoutException(
			"received timeout is less than maximum timeout of method");
	}

	const IQRFJsonResponse::Ptr response =
		IQRFUtil::makeRequest(
			m_connector,
			new DPACoordBondedNodesRequest,
			m_receiveTimeout
		);

	const DPACoordBondedNodesResponse::Ptr bondedNodes =
		DPAResponse::fromRaw(
			response->response()).cast<DPACoordBondedNodesResponse>();

	logger().information(
		"bonded nodes on the coordinator: "
		+ to_string(bondedNodes->decodeNodeBonded().size()),
		__FILE__, __LINE__);

	return bondedNodes->decodeNodeBonded();
}

set<uint8_t> IQRFDeviceManager::obtainNonSynchronizedNodes(
		const set<uint8_t> &bondedNodes)
{
	if (m_devices.empty())
		return bondedNodes;

	set<uint16_t> pairedNodes;
	for (const auto &device : m_devices)
		pairedNodes.insert(device.second->networkAddress());

	set<uint8_t> nonSynchronizedNodes;
	for (const auto &node : bondedNodes) {
		auto it = pairedNodes.find(node);

		if (it == pairedNodes.end())
			nonSynchronizedNodes.insert(node);
	}

	return nonSynchronizedNodes;
}

DPAProtocol::Ptr IQRFDeviceManager::detectNodeProtocol(
		DPAMessage::NetworkAddress node,
		const Timespan &maxMethodTimeout)
{
	for (auto protocol : m_dpaProtocols) {
		IQRFJsonResponse::Ptr response;

		if (logger().debug()) {
			logger().debug(
				"testing protocol "
				+ ClassInfo::forPointer(protocol.get()).name()
				+ " for node " + to_string(node),
				__FILE__, __LINE__);
		}

		if (m_receiveTimeout > maxMethodTimeout) {
			throw TimeoutException(
				"received timeout is less than maximum timeout of method");
		}

		try {
			response = IQRFUtil::makeRequest(
				m_connector,
				protocol->pingRequest(node),
				m_receiveTimeout
			);

			if (logger().debug()) {
				logger().debug(
					"protocol "
					+ ClassInfo::forPointer(protocol.get()).name()
					+ " has succeeded for node " + to_string(node));
			}

			return protocol;
		}
		catch (const TimeoutException &) {
			continue;
		}
		BEEEON_CATCH_CHAIN(logger())
	}

	throw ProtocolException(
		"device " + to_string(node) + " does not support any available protocol");
}

void IQRFDeviceManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	const Clock started;

	auto bondedNodes = obtainBondedNodes(METHOD_TIMEOUT - started.elapsed());
	auto nodes = obtainDeviceInfo(METHOD_TIMEOUT - started.elapsed(), bondedNodes);

	auto device = nodes.find(cmd->deviceID());
	if (device == end(nodes))
		throw NotFoundException("accept: " + cmd->deviceID().toString());

	FastMutex::ScopedLock guard(m_lock);
	auto it = m_devices.emplace(device->first, device->second);
	if (!it.second) {
		if (logger().debug()) {
			logger().debug(
				"device " + device->second->toString() + " is already registered",
				__FILE__, __LINE__);
		}
	}
	else {
		m_pollingKeeper.schedule(device->second);
	}

	DeviceManager::handleAccept(cmd);

	logger().information(
		"device " + cmd->deviceID().toString() + " has been paired",
		__FILE__, __LINE__);
}

void IQRFDeviceManager::newDevice(IQRFDevice::Ptr dev)
{
	const auto description = DeviceDescription::Builder()
		.id(dev->id())
		.type(dev->vendorName(), dev->productName())
		.modules(dev->modules())
		.build();

	if (logger().debug()) {
		logger().debug(
			"dispatching new device: " + dev->toString(),
			__FILE__, __LINE__);
	}

	dispatch(new NewDeviceCommand(description));
}

AsyncWork<>::Ptr IQRFDeviceManager::startDiscovery(
		const Timespan &timeout)
{
	m_bondingMode = true;
	Clock started;

	if ((timeout - IQRF_BONDING_TIME) < 0) {
		throw InvalidArgumentException(
			"given time for discovery is too short");
	}

	auto bondedNodes = obtainBondedNodes(timeout - started.elapsed());
	auto deices = obtainDeviceInfo(timeout - started.elapsed(), bondedNodes);
	for (const auto &dev : deices)
		newDevice(dev.second);

	while (!m_stopControl.shouldStop()) {
		if (timeout - IQRF_BONDING_TIME - started.elapsed() < 0)
			break;

		try {
			IQRFDevice::Ptr dev = bondNewDevice(timeout - started.elapsed());
			newDevice(dev);
			break;
		}
		catch (const Exception &ex) {
			continue;
		}
	}

	m_bondingMode = false;
	return BlockingAsyncWork<>::instance();
}

AsyncWork<set<DeviceID>>::Ptr IQRFDeviceManager::startUnpair(
		const DeviceID &id,
		const Timespan &timeout)
{
	Clock started;

	auto work = BlockingAsyncWork<set<DeviceID>>::instance();
	FastMutex::ScopedLock guard(m_lock);

	auto it = m_devices.find(id);
	if (it == m_devices.end()) {
		logger().warning(
			"attempt to unpair unknown device: "
			+ id.toString());

		return work;
	}

	const auto address = it->second->networkAddress();
	m_pollingKeeper.cancel(id);

	DPABatchRequest::Ptr request = new DPABatchRequest(address);
	request->append(new DPANodeRemoveBondRequest(address));
	request->append(new DPAOSRestartRequest(address));

	while (!started.isElapsed(timeout.totalMicroseconds())) {
		try {
			IQRFUtil::makeRequest(
				m_connector,
				request,
				m_receiveTimeout
			);

			if (logger().debug()) {
				logger().debug(
					"successfully removed node id "
					+ to_string(address) + " from node",
					__FILE__, __LINE__);
			}
			break;
		}
		catch (const TimeoutException &) {
			continue;
		}
		catch (...) {
			throw;
		}
	}

	MultiException me;
	while (!started.isElapsed(timeout.totalMicroseconds())) {
		try {
			IQRFUtil::makeRequest(
				m_connector,
				new DPACoordRemoveNodeRequest(address),
				m_receiveTimeout
			);

			if (logger().debug()) {
				logger().debug(
					"successfully removed node id "
					+ to_string(address) + " from coordinator",
					__FILE__, __LINE__);
			}
			break;
		}
		catch (const Exception &e) {
			logger().log(e);
			me.caught(e);

			if (me.count() > THRESHOLD_WRITE_EXCEPTION)
				me.rethrow();

			continue;
		}
	}

	m_devices.erase(it);
	deviceCache()->markUnpaired(id);
	work->setResult({id});

	if (logger().debug()) {
		logger().debug(
			"device " + id.toString() + " unpaired",
			__FILE__, __LINE__);
	}

	return work;
}

IQRFDevice::Ptr IQRFDeviceManager::bondNewDevice(
		const Timespan &timeout)
{
	if (logger().debug())
		logger().debug(
			"run bond new device (remains "
			+ to_string(timeout.totalSeconds()) + " s)",
			__FILE__, __LINE__);

	const IQRFJsonResponse::Ptr bondNodeResponse =
		IQRFUtil::makeRequest(
			m_connector,
			new DPACoordBondNodeRequest,
			timeout
		);

	if (logger().debug())
		logger().debug("run discovery new device", __FILE__, __LINE__);

	IQRFUtil::makeRequest(
		m_connector,
		new DPACoordDiscoveryRequest,
		timeout
	);

	const DPACoordBondNodeResponse::Ptr dpa
		= DPAResponse::fromRaw(bondNodeResponse->response())
			.cast<DPACoordBondNodeResponse>();

	const auto device = tryObtainDeviceInfo(
		dpa->bondedNetworkAddress(), timeout);

	return device;
}

void IQRFDeviceManager::notifyDongleRemoved()
{
	eraseAllDevices();
}

void IQRFDeviceManager::dongleFailed(const FailDetector &)
{
	eraseAllDevices();
}

void IQRFDeviceManager::eraseAllDevices()
{
	FastMutex::ScopedLock guard(m_lock);
	m_pollingKeeper.cancelAll();
	m_devices.clear();
}
