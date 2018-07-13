#include <fstream>
#include <iostream>
#include <sstream>

#include <Poco/Exception.h>

#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "core/Distributor.h"
#include "di/Injectable.h"
#include "fitp/FitpDevice.h"
#include "fitp/FitpDeviceManager.h"

BEEEON_OBJECT_BEGIN(BeeeOn, FitpDeviceManager)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_PROPERTY("deviceCache", &FitpDeviceManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("file", &FitpDeviceManager::setConfigPath)
BEEEON_OBJECT_PROPERTY("noiseMin", &FitpDeviceManager::setNoiseMin)
BEEEON_OBJECT_PROPERTY("noiseMax", &FitpDeviceManager::setNoiseMax)
BEEEON_OBJECT_PROPERTY("bitrate", &FitpDeviceManager::setBitrate)
BEEEON_OBJECT_PROPERTY("band", &FitpDeviceManager::setBand)
BEEEON_OBJECT_PROPERTY("channel", &FitpDeviceManager::setChannel)
BEEEON_OBJECT_PROPERTY("power", &FitpDeviceManager::setPower)
BEEEON_OBJECT_PROPERTY("txRetries", &FitpDeviceManager::setTxRetries)
BEEEON_OBJECT_PROPERTY("distributor", &FitpDeviceManager::setDistributor)
BEEEON_OBJECT_PROPERTY("gatewayInfo", &FitpDeviceManager::setGatewayInfo)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &FitpDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_HOOK("done", &FitpDeviceManager::initFitp)
BEEEON_OBJECT_END(BeeeOn, FitpDeviceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

#define COORDINATOR_READY    0xcc
#define END_DEVICE_READY     0x00
#define END_DEVICE_SLEEPY    0xff

#define EDID_LENGTH          4
#define EDID_OFFSET          2
#define EDID_START           2
#define EDID_END             6
#define EDID_MASK            0xffffffff

#define SKIP_INFO            6
#define FITP_DATA_OFFSET     6
#define JOIN_REQUEST_LENGTH  6

#define DEFAULT_REFRESH_TIME 60 * Timespan::SECONDS

#define PRODUCT_COORDINATOR "Temperature sensor"
#define PRODUCT_END_DEVICE  "Temperature and humidity sensor"
#define VENDOR              "BeeeOn"
#define FITP_CONFIG_PATH    "/var/cache/beeeon/gateway/fitp.devices"

FitpDeviceManager::FitpDeviceManager():
	DeviceManager(DevicePrefix::PREFIX_FITPROTOCOL, {
		typeid(GatewayListenCommand),
		typeid(DeviceUnpairCommand),
		typeid(DeviceAcceptCommand),
	}),
	m_configFile(FITP_CONFIG_PATH),
	m_listening(false),
	m_listenCallback(*this, &FitpDeviceManager::stopListen),
	m_listenTimer(0, 0)
{
}

FitpDeviceManager::~FitpDeviceManager()
{
	fitp_deinit();
}

void FitpDeviceManager::doListenCommand(const GatewayListenCommand::Ptr cmd)
{
	if (m_listening) {
		logger().debug(
			"listening is already in progress",
			__FILE__, __LINE__
		);
		return;
	}
	m_listening = true;

	if (cmd->duration().totalSeconds() < 1) {
		throw InvalidArgumentException(
			to_string(cmd->duration().totalMilliseconds())
			+ " ms");
	}

	logger().debug(
		"starting listening mode",
		__FILE__, __LINE__
	);

	fitp_listen(cmd->duration().totalSeconds());

	m_listenTimer.stop();
	m_listenTimer.setStartInterval(cmd->duration().totalMilliseconds());
	m_listenTimer.start(m_listenCallback);
}

void FitpDeviceManager::stopListen(Timer &)
{
	fitp_joining_disable();
	m_listening = false;

	logger().debug(
		"listening mode has finished",
		__FILE__, __LINE__
	);
}

void FitpDeviceManager::doDeviceAcceptCommand(const DeviceAcceptCommand::Ptr cmd)
{
	FastMutex::ScopedLock guard(m_lock);

	auto it = m_devices.find(cmd->deviceID());
	if (it == m_devices.end())
		throw NotFoundException("accept: " + cmd->deviceID().toString());

	if (deviceCache()->paired(cmd->deviceID())) {
		logger().warning(
			"ignoring accept for paired device "
			+ cmd->deviceID().toString(),
			__FILE__, __LINE__
		);
		return;
	}

	logger().debug(
		"accept command received successfully",
		__FILE__, __LINE__
	);

	deviceCache()->markPaired(cmd->deviceID());

	logger().notice(
		"device "
		+ cmd->deviceID().toString()
		+ " was successfully paired"
		__FILE__, __LINE__
	);
}

void FitpDeviceManager::doUnpairCommand(const DeviceUnpairCommand::Ptr cmd)
{
	FastMutex::ScopedLock guard(m_lock);
	auto it = m_devices.find(cmd->deviceID());
	if (it == m_devices.end() || !deviceCache()->paired(cmd->deviceID())) {
		logger().warning(
			"unpairing device that is not registered: "
			+ cmd->deviceID().toString(),
			__FILE__, __LINE__
		);
		return;
	}

	FitpDeviceManager::EDID edid  = deriveEDID(cmd->deviceID());

	if (!fitp_unpair(edid)) {
		throw IllegalStateException(
			"failed to unpair device "
			+ cmd->deviceID().toString());
	}

	logger().notice(
		"device "
		+ cmd->deviceID().toString()
		+ " was successfully unpaired"
			__FILE__, __LINE__
	);

	deviceCache()->markUnpaired(cmd->deviceID());
	m_devices.erase(cmd->deviceID());
}

void FitpDeviceManager::handleGeneric(const Command::Ptr cmd, Result::Ptr result)
{
	if (cmd->is<GatewayListenCommand>())
		doListenCommand(cmd.cast<GatewayListenCommand>());
	else if (cmd->is<DeviceUnpairCommand>())
		doUnpairCommand(cmd.cast<DeviceUnpairCommand>());
	else if (cmd->is<DeviceAcceptCommand>())
		doDeviceAcceptCommand(cmd.cast<DeviceAcceptCommand>());
	else
		DeviceManager::handleGeneric(cmd, result);
}

void FitpDeviceManager::dispatchNewDevice(FitpDevice::Ptr device)
{
	NewDeviceCommand::Ptr cmd;

	if (device->type() == FitpDevice::END_DEVICE) {
		cmd = new NewDeviceCommand(
			device->deviceID(),
			VENDOR,
			PRODUCT_END_DEVICE,
			device->modules(FitpDevice::END_DEVICE),
			DEFAULT_REFRESH_TIME);
	}
	else if (device->type() == FitpDevice::COORDINATOR) {
		cmd = new NewDeviceCommand(
			device->deviceID(),
			VENDOR,
			PRODUCT_COORDINATOR,
			device->modules(FitpDevice::COORDINATOR),
			DEFAULT_REFRESH_TIME);
	}
	else {
		throw InvalidArgumentException("invalid device type");
	}

	dispatch(cmd);
}

void FitpDeviceManager::setConfigPath(const string &configPath)
{
	m_configFile = configPath;
}

void FitpDeviceManager::setNoiseMin(int min)
{
	if (min < 0) {
		throw InvalidArgumentException(
			"invalid min: " + to_string(min));
	}
	m_phyParams.cca_noise_threshold_min = min;
}

void FitpDeviceManager::setNoiseMax(int max)
{
	if (max < 0) {
		throw InvalidArgumentException(
			"invalid max: " + to_string(max));
	}
	m_phyParams.cca_noise_threshold_max = max;
}

void FitpDeviceManager::setBitrate(int bitrate)
{
	if (bitrate < 0 || bitrate > 7) {
		throw InvalidArgumentException(
			"invalid bitrate: " + to_string(bitrate));
	}
	m_phyParams.bitrate = bitrate;
}

void FitpDeviceManager::setBand(int band)
{
	if (band < 0 || band > 3) {
		throw InvalidArgumentException(
			"invalid band: " + to_string(band));
	}
	m_phyParams.band = band;
}

int FitpDeviceManager::channelCnt()
{
	if ((m_phyParams.band == 0 || m_phyParams.band == 1) && (m_phyParams.bitrate == 6 || m_phyParams.bitrate == 7))
		return 24;
	else
		return 31;
}

void FitpDeviceManager::setChannel(int channel)
{
	if (channelCnt() < channel) {
			throw InvalidArgumentException(
				"invalid channel: " + to_string(channel));
	}

	m_phyParams.channel = channel;
}

void FitpDeviceManager::setPower(int power)
{
	if (power < 0 || power > 7) {
		throw InvalidArgumentException(
			"invalid TX power: " + to_string(power));
	}
	m_phyParams.power = power;
}

void FitpDeviceManager::setTxRetries(int retries)
{
	if (retries < 0) {
		throw InvalidArgumentException(
			"invalid retries: " + to_string(retries));
	}
	m_linkParams.tx_max_retries = retries;
}

void FitpDeviceManager::initFitp()
{
	logger().information(
		"configuration file path: "
		+ m_configFile,
		__FILE__, __LINE__
	);
	fitp_set_config_path(m_configFile);

	logger().debug(
		"acceptable noise: "
		+ to_string(m_phyParams.cca_noise_threshold_min)
		+ "-" + to_string(m_phyParams.cca_noise_threshold_max)
		+ ", bitrate: "
		+ to_string(m_phyParams.bitrate)
		+ ", band: "
		+ to_string(m_phyParams.band)
		+ ", channel: "
		+ to_string(m_phyParams.channel)
		+ ", TX power: "
		+ to_string(m_phyParams.power)
		+ ", TX retries: "
		+ to_string(m_linkParams.tx_max_retries),
		__FILE__, __LINE__
	);
	fitp_init(&m_phyParams, &m_linkParams);
}

void FitpDeviceManager::loadDeviceList()
{
	set<DeviceID> deviceIDs;

	try {
		deviceIDs = deviceList(-1);
	}
	catch (const Poco::Exception &ex) {
		logger().log(ex, __FILE__, __LINE__);
		return;
	}

	FastMutex::ScopedLock guard(m_lock);
	for (auto &item : deviceIDs) {
		FitpDevice::Ptr device = new FitpDevice(item);
		m_devices.emplace(item, device);
		deviceCache()->markPaired(item);
	}
}

DeviceID FitpDeviceManager::buildID(FitpDeviceManager::EDID edid)
{
	return DeviceID(DevicePrefix::PREFIX_FITPROTOCOL, edid);
}

FitpDeviceManager::EDID FitpDeviceManager::deriveEDID(const DeviceID &id)
{
	return id.ident() & EDID_MASK;
}

FitpDeviceManager::EDID FitpDeviceManager::parseEDID(const vector<uint8_t> &id)
{
	if (id.size() < EDID_LENGTH)
		throw InvalidArgumentException("invalid end device id length");

	EDID edid = 0;
	for (int i = 0; i < EDID_LENGTH; i++) {
		edid <<= 8;
		edid |= id.at(i);
	}
	return edid;
}

void FitpDeviceManager::processDataMsg(const vector<uint8_t> &data)
{
	logger().dump(
		"received data",
		data.data(),
		data.size(),
		Message::PRIO_TRACE
	);

	EDID edid = parseEDID({data.begin() + EDID_OFFSET, data.begin() + EDID_OFFSET + EDID_LENGTH});
	DeviceID deviceID = buildID(edid);

	FastMutex::ScopedLock guard(m_lock);
	auto it = m_devices.find(deviceID);
	if (it != m_devices.end() && deviceCache()->paired(deviceID)) {
		try {
			const vector<uint8_t> &values = {data.begin() + FITP_DATA_OFFSET, data.end()};
			SensorData sensorData = it->second->parseMessage(values, deviceID);
			ship(sensorData);
		}
		catch(const Exception &ex) {
			logger().log(ex, __FILE__, __LINE__);
		}
	}
	else {
		logger().warning(
			"data cannot be shipped, device "
			+ deviceID.toString()
			+ " is not paired",
			__FILE__, __LINE__
		);
	}
}

void FitpDeviceManager::processJoinMsg(const vector<uint8_t> &data)
{
	logger().dump(
		"join request",
		data.data(),
		data.size()
	);

	if(!m_listening) {
		logger().warning(
			"received join message out of the listen mode, ignoring...",
			__FILE__, __LINE__
		);
		return;
	}

	if (data.size() != JOIN_REQUEST_LENGTH) {
		logger().error(
			"invalid join request length",
			__FILE__, __LINE__
		);
		return;
	}

	EDID edid = parseEDID({data.begin() + EDID_OFFSET, data.begin() + EDID_OFFSET + EDID_LENGTH});
	DeviceID deviceID = buildID(edid);

	FastMutex::ScopedLock guard(m_lock);
	auto it = m_devices.find(deviceID);
	if (it == m_devices.end() || !deviceCache()->paired(deviceID)) {
		FitpDevice::Ptr device;

		switch (data[1]) {
		case END_DEVICE_READY:
		case END_DEVICE_SLEEPY:
		case COORDINATOR_READY:
			device = new FitpDevice(deviceID);
			break;
		default:
			logger().warning(
				"invalid device type "
				+ to_string(data[1]),
				__FILE__, __LINE__
			);
			break;
		}

		if (data[1] == END_DEVICE_READY || data[1] == END_DEVICE_SLEEPY || data[1] == COORDINATOR_READY) {
			device = new FitpDevice(deviceID);
		}
		else {
			logger().warning(
				"invalid device type",
				__FILE__, __LINE__
			);
			return;
		}

		device->setDeviceID(deviceID);

		fitp_accepted_device({data.begin() + EDID_START, data.begin() + EDID_END});
		m_devices.emplace(device->deviceID(), device);
		dispatchNewDevice(device);
	}
	else {
		logger().warning(
			"device "
			+ deviceID.toString()
			+ " has been already paired",
			__FILE__, __LINE__
		);
	}
}

void FitpDeviceManager::setGatewayInfo(Poco::SharedPtr<GatewayInfo> info)
{
	m_gatewayInfo = info;
}

void FitpDeviceManager::run()
{
	loadDeviceList();

	logger().information(
		"starting fitp device manager, fitplib version: "
		+ fitp_version(),
		__FILE__, __LINE__
	);

	fitp_set_nid((uint32_t) m_gatewayInfo->gatewayID().data());

	StopControl::Run run(m_stopControl);

	while (run) {
		vector<uint8_t> data;

		fitp_received_data(data);
		if (!data.empty()) {
			if (isDataMessage(data)) {
				processDataMsg(data);
			}
			else if (isJoinMessage(data)) {
				processJoinMsg(data);
			}
		}
	}

}

void FitpDeviceManager::stop()
{
	m_listenTimer.stop();
	if (m_listening) {
		fitp_joining_disable();
		m_listening = false;
	}

	DeviceManager::stop();
}

