#include <Poco/AutoPtr.h>
#include <Poco/Timestamp.h>
#include <Poco/Crypto/Cipher.h>
#include <Poco/Crypto/CipherFactory.h>
#include <Poco/Crypto/CipherKey.h>
#include <Poco/Net/IPAddress.h>

#include "core/CommandDispatcher.h"
#include "credentials/PasswordCredentials.h"
#include "di/Injectable.h"
#include "model/DevicePrefix.h"
#include "net/VPTHTTPScanner.h"
#include "vpt/VPTDeviceManager.h"

BEEEON_OBJECT_BEGIN(BeeeOn, VPTDeviceManager)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(CommandHandler)
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
	m_seeker(*this),
	m_maxMsgSize(10000),
	m_refresh(5 * Timespan::SECONDS),
	m_httpTimeout(3 * Timespan::SECONDS),
	m_pingTimeout(20 * Timespan::MILLISECONDS)
{
}

void VPTDeviceManager::run()
{
	logger().information("starting VPT device manager");

	try {
		m_pairedDevices = deviceList(-1);
	}
	catch (Exception& e) {
		logger().log(e, __FILE__, __LINE__);
	}
	if (m_pairedDevices.size() > 0)
		searchPairedDevices();

	while (!m_stop) {
		Timestamp now;

		shipFromDevices();

		Timespan sleepTime = m_refresh - now.elapsed();
		if (sleepTime > 0) {
			logger().debug("sleeping for " + to_string(sleepTime.totalMilliseconds()) + " ms",
				__FILE__, __LINE__);
			m_event.tryWait(sleepTime.totalMilliseconds());
		}
	}

	logger().information("stopping VPT device manager");
	m_stop = false;
}

void VPTDeviceManager::stop()
{
	m_stop = true;
	m_scanner.cancel();
	m_seeker.stop();
	m_event.set();
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
	for (auto &id : m_pairedDevices) {
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
			auto it = m_pairedDevices.find(one.deviceID());
			if (it != m_pairedDevices.end())
				ship(one);
		}
	}
}

void VPTDeviceManager::searchPairedDevices()
{
	vector<VPTDevice::Ptr> devices = seekDevices();

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

		auto itPaired = m_pairedDevices.find(subDevID);
		if (itPaired != m_pairedDevices.end())
			return true;
	}

	return false;
}

void VPTDeviceManager::handle(Command::Ptr cmd, Answer::Ptr answer)
{
	if (cmd->is<GatewayListenCommand>()) {
		doListenCommand(cmd.cast<GatewayListenCommand>(), answer);
	}
	else if (cmd->is<DeviceSetValueCommand>()) {
		modifyValue(cmd.cast<DeviceSetValueCommand>(), answer);
	}
	else if (cmd->is<DeviceUnpairCommand>()) {
		doUnpairCommand(cmd.cast<DeviceUnpairCommand>(), answer);
	}
	else if (cmd->is<DeviceAcceptCommand>()) {
		doDeviceAcceptCommand(cmd.cast<DeviceAcceptCommand>(), answer);
	}
}

void VPTDeviceManager::doListenCommand(const GatewayListenCommand::Ptr cmd, const Answer::Ptr answer)
{
	Result::Ptr result = new Result(answer);

	if (m_seeker.startSeeking(cmd->duration()))
		result->setStatus(Result::Status::SUCCESS);
	else
		result->setStatus(Result::Status::FAILED);
}

void VPTDeviceManager::doUnpairCommand(const DeviceUnpairCommand::Ptr cmd, const Answer::Ptr answer)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	auto it = m_pairedDevices.find(cmd->deviceID());
	if (it == m_pairedDevices.end()) {
		logger().warning("unpairing device that is not paired: " + cmd->deviceID().toString(),
			__FILE__, __LINE__);
	}
	else {
		m_pairedDevices.erase(it);

		DeviceID tmpID = VPTDevice::omitSubdeviceFromDeviceID(cmd->deviceID());
		if (noSubdevicePaired(tmpID))
			m_devices.erase(tmpID);
	}

	Result::Ptr result = new Result(answer);
	result->setStatus(Result::Status::SUCCESS);
}

void VPTDeviceManager::doDeviceAcceptCommand(const DeviceAcceptCommand::Ptr cmd, const Answer::Ptr answer)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	Result::Ptr result = new Result(answer);

	auto it = m_devices.find(VPTDevice::omitSubdeviceFromDeviceID(cmd->deviceID()));
	if (it == m_devices.end()) {
		logger().warning("not accepting device that is not found: " + cmd->deviceID().toString(),
			__FILE__, __LINE__);
		result->setStatus(Result::Status::FAILED);
		return;
	}

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

	m_pairedDevices.insert(cmd->deviceID());

	result->setStatus(Result::Status::SUCCESS);
}

void VPTDeviceManager::modifyValue(const DeviceSetValueCommand::Ptr cmd, const Answer::Ptr answer)
{
	FastMutex::ScopedLock lock(m_pairedMutex);

	Result::Ptr result = new Result(answer);
	try {
		auto it = m_devices.find(VPTDevice::omitSubdeviceFromDeviceID(cmd->deviceID()));
		if (it == m_devices.end()) {
			logger().warning("no such device: " + cmd->deviceID().toString(), __FILE__, __LINE__);
		}
		else {
			ScopedLock<FastMutex> guard(it->second->lock());
			it->second->requestModifyState(cmd->deviceID(), cmd->moduleID(), cmd->value());
			result->setStatus(Result::Status::SUCCESS);

			SensorData data;
			data.setDeviceID(cmd->deviceID());
			data.insertValue({cmd->moduleID(), cmd->value()});
			ship(data);
			return;
		}
	}
	catch (const Exception& e) {
		logger().log(e, __FILE__, __LINE__);
		logger().warning("failed to change state of device " + cmd->deviceID().toString(),
			__FILE__, __LINE__);
	}
	catch (...) {
		logger().critical("unexpected exception", __FILE__, __LINE__);
	}

	result->setStatus(Result::Status::FAILED);
}

bool VPTDeviceManager::noSubdevicePaired(const DeviceID& id) const
{
	for (int i = 0; i <= VPTDevice::COUNT_OF_ZONES; i++) {
		auto it = m_pairedDevices.find(VPTDevice::createSubdeviceID(i, id));
		if (it != m_pairedDevices.end())
			return false;
	}

	return true;
}

vector<VPTDevice::Ptr> VPTDeviceManager::seekDevices()
{
	vector<VPTDevice::Ptr> devices;
	vector<SocketAddress> list = m_scanner.scan(m_maxMsgSize);

	for (auto& address : list) {
		if (m_stop)
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

	vector<NewDeviceCommand::Ptr> newDeviceCommands = newDevice->createNewDeviceCommands(m_refresh);
	for (auto cmd : newDeviceCommands) {
		auto pairedDevice = m_pairedDevices.find(cmd->deviceID());
		if (pairedDevice == m_pairedDevices.end())
			dispatch(cmd);
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

VPTDeviceManager::VPTSeeker::VPTSeeker(VPTDeviceManager& parent):
	m_parent(parent),
	m_stop(false)
{
}

VPTDeviceManager::VPTSeeker::~VPTSeeker()
{
	/*
	 * Scanning of device can be in progress, it should take up to httpTimeout.
	 */
	m_seekerThread.join(m_parent.m_httpTimeout.totalMilliseconds());
}

bool VPTDeviceManager::VPTSeeker::startSeeking(const Timespan& duration)
{
	m_duration = duration;

	if (!m_seekerThread.isRunning()) {
		try {
			m_seekerThread.start(*this);
		}
		catch (const Exception &e) {
			m_parent.logger().log(e, __FILE__, __LINE__);
			m_parent.logger().error("listening thread failed to start", __FILE__, __LINE__);
			return false;
		}
	}
	else {
		m_parent.logger().debug("listen seems to be running already, dropping listen command", __FILE__, __LINE__);
	}

	return true;
}

void VPTDeviceManager::VPTSeeker::run()
{
	Timestamp now;

	while (now.elapsed() < m_duration.totalMicroseconds()) {
		for (auto device : m_parent.seekDevices()) {
			if (m_stop)
				break;

			m_parent.processNewDevice(device);
		}

		if (m_stop)
			break;
	}

	m_stop = false;
}

void VPTDeviceManager::VPTSeeker::stop()
{
	m_stop = true;
}
