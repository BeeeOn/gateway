#include <Poco/AutoPtr.h>
#include <Poco/Timestamp.h>
#include <Poco/Crypto/Cipher.h>
#include <Poco/Crypto/CipherFactory.h>
#include <Poco/Crypto/CipherKey.h>
#include <Poco/Net/IPAddress.h>

#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "credentials/PasswordCredentials.h"
#include "di/Injectable.h"
#include "model/DevicePrefix.h"
#include "net/VPTHTTPScanner.h"
#include "util/BlockingAsyncWork.h"
#include "vpt/VPTDeviceManager.h"

BEEEON_OBJECT_BEGIN(BeeeOn, VPTDeviceManager)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(DeviceStatusHandler)
BEEEON_OBJECT_PROPERTY("deviceCache", &VPTDeviceManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("distributor", &VPTDeviceManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &VPTDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("refresh", &VPTDeviceManager::setRefresh)
BEEEON_OBJECT_PROPERTY("interfaceBlackList", &VPTDeviceManager::setBlackList)
BEEEON_OBJECT_PROPERTY("pingTimeout", &VPTDeviceManager::setPingTimeout)
BEEEON_OBJECT_PROPERTY("httpTimeout", &VPTDeviceManager::setHTTPTimeout)
BEEEON_OBJECT_PROPERTY("maxMsgSize", &VPTDeviceManager::setMaxMsgSize)
BEEEON_OBJECT_PROPERTY("path", &VPTDeviceManager::setPath)
BEEEON_OBJECT_PROPERTY("port", &VPTDeviceManager::setPort)
BEEEON_OBJECT_PROPERTY("minNetMask", &VPTDeviceManager::setMinNetMask)
BEEEON_OBJECT_PROPERTY("gatewayInfo", &VPTDeviceManager::setGatewayInfo)
BEEEON_OBJECT_PROPERTY("credentialsStorage", &VPTDeviceManager::setCredentialsStorage)
BEEEON_OBJECT_PROPERTY("cryptoConfig", &VPTDeviceManager::setCryptoConfig)
BEEEON_OBJECT_END(BeeeOn, VPTDeviceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Crypto;
using namespace Poco::Net;
using namespace std;

VPTDeviceManager::VPTDeviceManager():
	DeviceManager(DevicePrefix::PREFIX_VPT, {
		typeid(GatewayListenCommand),
		typeid(DeviceAcceptCommand),
		typeid(DeviceUnpairCommand),
		typeid(DeviceSetValueCommand),
	}),
	m_maxMsgSize(10000),
	m_refresh(5 * Timespan::SECONDS),
	m_httpTimeout(3 * Timespan::SECONDS),
	m_pingTimeout(20 * Timespan::MILLISECONDS)
{
}

void VPTDeviceManager::run()
{
	logger().information("starting VPT device manager");

	set<DeviceID> paired = waitRemoteStatus(-1);

	if (paired.size() > 0)
		searchPairedDevices();

	StopControl::Run run(m_stopControl);

	while (run) {
		Timestamp now;

		shipFromDevices();

		Timespan sleepTime = m_refresh - now.elapsed();
		if (sleepTime > 0) {
			logger().debug("sleeping for " + to_string(sleepTime.totalMilliseconds()) + " ms",
				__FILE__, __LINE__);
			run.waitStoppable(sleepTime);
		}
	}

	logger().information("stopping VPT device manager");
}

void VPTDeviceManager::stop()
{
	m_scanner.cancel();
	DeviceManager::stop();
	answerQueue().dispose();
}

void VPTDeviceManager::setRefresh(const Timespan &refresh)
{
	if (refresh.totalSeconds() <= 0)
		throw InvalidArgumentException("refresh time must be at least 1 second");

	m_refresh = refresh;
}

void VPTDeviceManager::setPingTimeout(const Timespan &timeout)
{
	if (timeout.totalMilliseconds() <= 0)
		throw InvalidArgumentException("ping timeout time must be at least 1 ms");

	m_scanner.setPingTimeout(timeout);
}

void VPTDeviceManager::setHTTPTimeout(const Timespan &timeout)
{
	if (timeout.totalMilliseconds() <= 0)
		throw InvalidArgumentException("HTTP timeout time must be at least 1 ms");

	m_httpTimeout = timeout;
	m_scanner.setHTTPTimeout(timeout);
}

void VPTDeviceManager::setMaxMsgSize(int size)
{
	if (size <= 0)
		throw InvalidArgumentException("max message size must be a positive number");

	m_maxMsgSize = size;
}

void VPTDeviceManager::setBlackList(const list<string>& list)
{
	set<string> blackList;
	copy(list.begin(), list.end(), std::inserter(blackList, blackList.end()));

	m_scanner.setBlackList(blackList);
}

void VPTDeviceManager::setPath(const string& path)
{
	m_scanner.setPath(path);
}

void VPTDeviceManager::setPort(const int port)
{
	if (port <= 0 || 65535 < port)
		throw InvalidArgumentException("invalid port " + to_string(port));

	m_scanner.setPort(port);
}

void VPTDeviceManager::setMinNetMask(const string& minNetMask)
{
	m_scanner.setMinNetMask(IPAddress(minNetMask));
}

void VPTDeviceManager::setGatewayInfo(SharedPtr<GatewayInfo> gatewayInfo)
{
	m_gatewayInfo = gatewayInfo;
}

void VPTDeviceManager::setCredentialsStorage(SharedPtr<CredentialsStorage> storage)
{
	m_credentialsStorage = storage;
}

void VPTDeviceManager::setCryptoConfig(SharedPtr<CryptoConfig> config)
{
	m_cryptoConfig = config;
}

void VPTDeviceManager::shipFromDevices()
{
	set<VPTDevice::Ptr> devices;

	ScopedLockWithUnlock<FastMutex> lock(m_pairedMutex);
	for (auto &id : deviceCache()->paired(prefix())) {
		DeviceID realVPTId = VPTDevice::omitSubdeviceFromDeviceID(id);
		auto itDevice = m_devices.find(realVPTId);

		if (itDevice == m_devices.end()) {
			logger().warning("no such device: " + id.toString(), __FILE__, __LINE__);
			continue;
		}

		auto it = devices.find(m_devices.at(realVPTId));
		if (it == devices.end())
			devices.insert(m_devices.at(realVPTId));
	}
	lock.unlock();

	for (auto device : devices) {
		vector<SensorData> data;
		try {
			ScopedLock<FastMutex> guard(device->lock());
			data = device->requestValues();
		}
		catch (const Exception& e) {
			logger().log(e, __FILE__, __LINE__);
			logger().warning("device " + device->boilerID().toString() + " did not answer",
				__FILE__, __LINE__);
			continue;
		}

		ScopedLock<FastMutex> guard(m_pairedMutex);
		for (auto &one : data) {
			if (deviceCache()->paired(one.deviceID()))
				ship(one);
		}
	}
}

void VPTDeviceManager::searchPairedDevices()
{
	vector<VPTDevice::Ptr> devices = seekDevices(m_stopControl);

	ScopedLock<FastMutex> lock(m_pairedMutex);
	for (auto device : devices) {
		if (isAnySubdevicePaired(device)) {
			auto it = m_devices.emplace(device->boilerID(), device);
			if (!it.second)
				continue;

			try {
				string password = findPassword(device->boilerID());

				ScopedLock<FastMutex> guard(device->lock());
				device->setPassword(password);
			}
			catch (NotFoundException& e) {
				logger().log(e, __FILE__, __LINE__);
			}
		}
	}
}

bool VPTDeviceManager::isAnySubdevicePaired(VPTDevice::Ptr device)
{
	// 0 is boiler
	for (int i = 0; i <= VPTDevice::COUNT_OF_ZONES; i++) {
		DeviceID subDevID = VPTDevice::createSubdeviceID(i, device->boilerID());

		if (deviceCache()->paired(subDevID))
			return true;
	}

	return false;
}

void VPTDeviceManager::handleGeneric(const Command::Ptr cmd, Result::Ptr result)
{
	if (cmd->is<DeviceSetValueCommand>()) {
		modifyValue(cmd.cast<DeviceSetValueCommand>());
	}
	else {
		DeviceManager::handleGeneric(cmd, result);
	}
}

AsyncWork<>::Ptr VPTDeviceManager::startDiscovery(const Timespan &timeout)
{
	VPTSeeker::Ptr seeker = new VPTSeeker(*this, timeout);
	seeker->start();
	return seeker;
}

AsyncWork<set<DeviceID>>::Ptr VPTDeviceManager::startUnpair(
		const DeviceID &id,
		const Timespan &)
{
	auto work = BlockingAsyncWork<set<DeviceID>>::instance();

	FastMutex::ScopedLock lock(m_pairedMutex);

	if (!deviceCache()->paired(id)) {
		logger().warning("unpairing device that is not paired: " + id.toString(),
			__FILE__, __LINE__);
	}
	else {
		deviceCache()->markUnpaired(id);

		DeviceID tmpID = VPTDevice::omitSubdeviceFromDeviceID(id);
		if (noSubdevicePaired(tmpID))
			m_devices.erase(tmpID);

		work->setResult({id});
	}

	return work;
}

void VPTDeviceManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	auto it = m_devices.find(VPTDevice::omitSubdeviceFromDeviceID(cmd->deviceID()));
	if (it == m_devices.end())
		throw NotFoundException("accept: " + cmd->deviceID().toString());

	/*
	 * The password is searched only when the first subdevice is accepted.
	 */
	if (noSubdevicePaired(VPTDevice::omitSubdeviceFromDeviceID(cmd->deviceID()))) {
		try {
			string password = findPassword(
				VPTDevice::omitSubdeviceFromDeviceID(cmd->deviceID()));

			ScopedLock<FastMutex> guard(it->second->lock());
			it->second->setPassword(password);
		}
		catch (NotFoundException& e) {
			logger().log(e, __FILE__, __LINE__);
		}
	}

	DeviceManager::handleAccept(cmd);
}

void VPTDeviceManager::modifyValue(const DeviceSetValueCommand::Ptr cmd)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	auto it = m_devices.find(VPTDevice::omitSubdeviceFromDeviceID(cmd->deviceID()));
	if (it == m_devices.end())
		throw NotFoundException("set-value: " + cmd->deviceID().toString());

	ScopedLock<FastMutex> guard(it->second->lock());
	it->second->requestModifyState(cmd->deviceID(), cmd->moduleID(), cmd->value());

	try {
		SensorData data;
		data.setDeviceID(cmd->deviceID());
		data.insertValue({cmd->moduleID(), cmd->value()});

		ship(data);
	}
	BEEEON_CATCH_CHAIN(logger())
}

bool VPTDeviceManager::noSubdevicePaired(const DeviceID& id) const
{
	for (int i = 0; i <= VPTDevice::COUNT_OF_ZONES; i++) {
		if (deviceCache()->paired(VPTDevice::createSubdeviceID(i, id)))
			return false;
	}

	return true;
}

vector<VPTDevice::Ptr> VPTDeviceManager::seekDevices(const StopControl& stop)
{
	vector<VPTDevice::Ptr> devices;
	vector<SocketAddress> list = m_scanner.scan(m_maxMsgSize);

	for (auto& address : list) {
		if (stop.shouldStop())
			break;

		VPTDevice::Ptr newDevice;
		try {
			newDevice = VPTDevice::buildDevice(address, m_httpTimeout,
				m_pingTimeout, m_gatewayInfo->gatewayID());
		}
		catch (Exception& e) {
			logger().warning("found device has disconnected", __FILE__, __LINE__);
			continue;
		}

		devices.push_back(newDevice);
	}

	return devices;
}

void VPTDeviceManager::processNewDevice(VPTDevice::Ptr newDevice)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	/*
	 * Finds out if the device is already added.
	 * If the device already exists but has different IP address
	 * update the device.
	 */
	auto it = m_devices.emplace(newDevice->boilerID(), newDevice);
	if (!it.second) {
		auto existingDevice = it.first->second;
		ScopedLock<FastMutex> guard(existingDevice->lock());

		existingDevice->setAddress(newDevice->address());
		return;
	}

	logger().debug("found device " + newDevice->boilerID().toString() +
		" at " + newDevice->address().toString(), __FILE__, __LINE__);

	const auto &descriptions = newDevice->descriptions(m_refresh);
	for (auto d : descriptions) {
		if (!deviceCache()->paired(d.id()))
			dispatch(new NewDeviceCommand(d));
	}
}

string VPTDeviceManager::findPassword(const DeviceID& id)
{
	SharedPtr<Credentials> credential = m_credentialsStorage->find(id);

	if (!credential.isNull()) {
		CipherKey key = m_cryptoConfig->createKey(credential->params());
		AutoPtr<Cipher> cipher = CipherFactory::defaultFactory().createCipher(key);

		if (!credential.cast<PasswordCredentials>().isNull()) {
			SharedPtr<PasswordCredentials> password = credential.cast<PasswordCredentials>();
			return password->password(cipher);
		}
	}

	throw NotFoundException("password not found for VPT " + id.toString());
}

VPTDeviceManager::VPTSeeker::VPTSeeker(VPTDeviceManager& parent, const Timespan& duration):
	AbstractSeeker(duration),
	m_parent(parent)
{
}

void VPTDeviceManager::VPTSeeker::seekLoop(StopControl &control)
{
	StopControl::Run run(control);

	while (remaining() > 0) {
		for (auto device : m_parent.seekDevices(control)) {
			if (!run)
				break;

			m_parent.processNewDevice(device);
		}

		if (!run)
			break;
	}
}
