#include <Poco/RegularExpression.h>
#include <Poco/Thread.h>
#include <Poco/StringTokenizer.h>

#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "io/AutoClose.h"
#include "jablotron/JablotronDeviceAC88.h"
#include "jablotron/JablotronDeviceManager.h"
#include "hotplug/HotplugEvent.h"
#include "util/BlockingAsyncWork.h"
#include "util/DelayedAsyncWork.h"

BEEEON_OBJECT_BEGIN(BeeeOn, JablotronDeviceManager)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(HotplugListener)
BEEEON_OBJECT_PROPERTY("deviceCache", &JablotronDeviceManager::setDeviceCache)
BEEEON_OBJECT_PROPERTY("distributor", &JablotronDeviceManager::setDistributor)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &JablotronDeviceManager::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("attemptsCount", &JablotronDeviceManager::setAttemptsCount)
BEEEON_OBJECT_PROPERTY("retryTimeout", &JablotronDeviceManager::setRetryTimeout)
BEEEON_OBJECT_END(BeeeOn, JablotronDeviceManager)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const string JABLOTRON_VENDOR_ID = "0403";
static const string JABLOTRON_PRODUCT_ID = "6015";
static const string VENDOR_NAME = "Jablotron";
static const int BAUD_RATE = 57600;
static const int MAX_DEVICES_IN_JABLOTRON = 32;
static const int NUMBER_OF_RETRIES = 3;
static const int MAX_NUMBER_FAILED_REPEATS = 10;
static const int RESPONSE_WAIT_MSEC = 5000;
static const int DEFAULT_PG_VALUE = 0;
static const Timespan READ_TIMEOUT = 1 * Timespan::SECONDS;
static const Timespan SLEEP_AFTER_FAILED = 1 * Timespan::SECONDS;
static const Timespan DELAY_AFTER_SET_SWITCH = 1 * Timespan::SECONDS;
static const Timespan DELAY_BEETWEEN_CYCLES = 300 * Timespan::MILLISECONDS;
static const DeviceID DEFAULT_DEVICE_ID(DevicePrefix::PREFIX_JABLOTRON, 0);


JablotronDeviceManager::JablotronDeviceManager():
	DongleDeviceManager(DevicePrefix::PREFIX_JABLOTRON, {
		typeid(GatewayListenCommand),
		typeid(DeviceAcceptCommand),
		typeid(DeviceUnpairCommand),
		typeid(DeviceSetValueCommand),
	}),
	m_lastResponse(NONE),
	m_isListen(false),
	m_pgx(DEFAULT_DEVICE_ID, DEFAULT_PG_VALUE),
	m_pgy(DEFAULT_DEVICE_ID, DEFAULT_PG_VALUE)
{
}

void JablotronDeviceManager::initJablotronDongle()
{
	m_serial.setBaudRate(BAUD_RATE);
	m_serial.setStopBits(SerialPort::StopBits::STOPBITS_1);
	m_serial.setParity(SerialPort::Parity::PARITY_NONE);
	m_serial.setDataBits(SerialPort::DataBits::DATABITS_8);
	m_serial.setNonBlocking(true);

	m_serial.setDevicePath(dongleName());
	m_serial.open();
	m_serial.flush();
}

void JablotronDeviceManager::dongleVersion()
{
	string message;

	m_serial.write("\x1BWHO AM I?\n");

	if (nextMessage(message) != MessageType::DATA) {
		logger().warning(
			"unexpected response: " + message,
			__FILE__, __LINE__);
		return;
	}

	if (wasTurrisDongleVersion(message)) {
		logger().information(
			"Jablotron Dongle version: "
			+ message,
		__FILE__, __LINE__);
	}
	else {
		logger().warning(
			"unknown dongle version: " + message,
		__FILE__, __LINE__);
	}
}

void JablotronDeviceManager::dongleAvailable()
{
	AutoClose<SerialPort> serial(m_serial);
	m_devices.clear();

	initJablotronDongle();
	dongleVersion();
	jablotronProcess();
}

void JablotronDeviceManager::stop()
{
	DongleDeviceManager::stop();
	answerQueue().dispose();
}

void JablotronDeviceManager::jablotronProcess()
{
	string message;

	setupDongleDevices();
	loadDeviceList();

	while (!m_stopControl.shouldStop()) {
		MessageType response = nextMessage(message);

		if (isResponse(response)) {
			m_lastResponse = response;
			m_responseRcv.set();
			continue;
		}

		if (response != MessageType::DATA) {
			logger().warning(
				"unexpected response: " + message,
				__FILE__, __LINE__);
			continue;
		}

		DeviceID id = JablotronDevice::buildID(extractSerialNumber(message));
		Mutex::ScopedLock guard(m_lock);

		auto it = m_devices.find(id);
		if (it != m_devices.end() && !it->second.isNull() && deviceCache()->paired(id)) {
			shipMessage(message, it->second);
		}
		else if (it != m_devices.end() && m_isListen) {
			doNewDevice(id, it);
		}
		else {
			logger().debug(
				"device " + id.toString()
				+ " is not paired",
				__FILE__, __LINE__);
		}
	}
}

void JablotronDeviceManager::handleAccept(const DeviceAcceptCommand::Ptr cmd)
{
	Mutex::ScopedLock guard(m_lock);

	auto it = m_devices.find(cmd->deviceID());
	if (it == m_devices.end())
		throw NotFoundException("accept: " + cmd->deviceID().toString());

	DeviceManager::handleAccept(cmd);
}

AsyncWork<>::Ptr JablotronDeviceManager::startDiscovery(const Timespan &timeout)
{
	m_isListen = true;

	auto finish = [&]() {
		logger().debug("listen is done", __FILE__, __LINE__);
		m_isListen = false;

		Mutex::ScopedLock guard(m_lock);
		for (auto &device : m_devices) {
			if (device.second.isNull())
				continue;

			if (!deviceCache()->paired(device.second->deviceID()))
				device.second = nullptr;
		}
	};

	return new DelayedAsyncWork<>(finish, finish, timeout);
}

AsyncWork<set<DeviceID>>::Ptr JablotronDeviceManager::startUnpair(
		const DeviceID &id,
		const Timespan &)
{
	set<DeviceID> result;

	auto work = BlockingAsyncWork<set<DeviceID>>::instance();

	Mutex::ScopedLock guard(m_lock);
	auto it = m_devices.find(id);

	if (it != m_devices.end()) {
		it->second = nullptr;
		deviceCache()->markUnpaired(id);
		work->setResult({id});
	}
	else {
		logger().warning(
			"attempt to unpair unknown device: "
			+ id.toString());
	}

	return work;
}

void JablotronDeviceManager::doNewDevice(const DeviceID &deviceID,
	map<DeviceID, JablotronDevice::Ptr>::iterator &it)
{
	if (it->second.isNull()) {
		try {
			it->second = JablotronDevice::create(deviceID.ident());
		}
		catch (const InvalidArgumentException &ex) {
			logger().log(ex, __FILE__, __LINE__);
			return;
		}
	}

	NewDeviceCommand::Ptr cmd =
		new NewDeviceCommand(
			it->second->deviceID(),
			VENDOR_NAME,
			it->second->name(),
			it->second->moduleTypes(),
			it->second->refreshTime()
		);

	dispatch(cmd);
}

void JablotronDeviceManager::loadDeviceList()
{
	set<DeviceID> deviceIDs = deviceList(-1);

	Mutex::ScopedLock guard(m_lock);
	for (auto &id : deviceIDs) {
		auto it = m_devices.find(id);

		if (it != m_devices.end()) {
			try {
				it->second = JablotronDevice::create(id.ident());
			}
			catch (const InvalidArgumentException &ex) {
				logger().log(ex, __FILE__, __LINE__);
				continue;
			}

			deviceCache()->markPaired(id);
		}
	}

	obtainLastValue();
}

JablotronDeviceManager::MessageType JablotronDeviceManager::nextMessage(
			string &message)
{
	// find message between newline
	// message example: \nAC-88 RELAY:1\n
	const RegularExpression re("(?!\\n)[^\\n]*(?=\\n)");

	while (!re.extract(m_buffer, message) && !m_stopControl.shouldStop()) {
		try {
			m_buffer += m_serial.read(READ_TIMEOUT);
		}
		catch (const TimeoutException &ex) {
			// avoid frozen state, allow to test shouldStop() time after time
			continue;
		}
	}

	// erase matched message from buffer + 2x newline
	m_buffer.erase(0, message.size() + 2);
	logger().trace("received data: " + message, __FILE__, __LINE__);

	return messageType(message);
}

JablotronDeviceManager::MessageType JablotronDeviceManager::messageType(
	const string &data)
{
	if (data == "OK")
		return MessageType::OK;
	else if (data == "ERROR")
		return MessageType::ERROR;

	return MessageType::DATA;
}

bool JablotronDeviceManager::wasTurrisDongleVersion(
	const string &message) const
{
	const RegularExpression re("TURRIS DONGLE V[0-9].[0-9]*");
	return re.match(message);
}

void JablotronDeviceManager::setupDongleDevices()
{
	string buffer;
	for (int i = 0; i < MAX_DEVICES_IN_JABLOTRON; i++) {
		Mutex::ScopedLock guard(m_lock);

		// we need 2-digits long value (zero-prefixed when needed)
		m_serial.write("\x1BGET SLOT:" + NumberFormatter::format0(i, 2) + "\n");

		if (nextMessage(buffer) != MessageType::DATA) {
			logger().error(
				"unknown message: " + buffer,
				__FILE__, __LINE__
			);

			continue;
		}

		// finding empty slot: SLOT:XX [--------]
		RegularExpression re(".*[--------].*");
		if (re.match(buffer)) {
			logger().trace("empty slot", __FILE__, __LINE__);
			continue;
		}

		try {
			uint32_t serialNumber = extractSerialNumber(buffer);
			DeviceID deviceID = JablotronDevice::buildID(serialNumber);

			// create empty JablotronDevice
			// after the obtain server device list, create JablotronDevice for paired devices
			m_devices.emplace(deviceID, nullptr);

			logger().debug(
				"created device: " + to_string(serialNumber)
				+ " from dongle", __FILE__, __LINE__);

			if (JablotronDevice::create(serialNumber).cast<JablotronDeviceAC88>().isNull())
				continue;

			if (m_pgx.first == DEFAULT_DEVICE_ID)
				m_pgx.first = deviceID;
			else
				m_pgy.first = deviceID;
		}
		catch (const InvalidArgumentException &ex) {
			logger().log(ex, __FILE__, __LINE__);
		}
		catch (const SyntaxException &ex) {
			logger().log(ex, __FILE__, __LINE__);
		}
	}
}

uint32_t JablotronDeviceManager::extractSerialNumber(const string &message)
{
	RegularExpression re("\\[([0-9]+)\\]");
	vector<string> tmp;

	if (re.split(message, tmp) == 2)
		return NumberParser::parseUnsigned(tmp[1]);

	throw InvalidArgumentException(
		"invalid serial number string: " + message);
}

void JablotronDeviceManager::shipMessage(
	const string &message, JablotronDevice::Ptr jablotronDevice)
{
	try {
		ship(jablotronDevice->extractSensorData(message));
	}
	catch (const RangeException &ex) {
		logger().error(
			"unexpected token index in message: " + message,
			__FILE__, __LINE__
		);
	}
	catch (const Exception &ex) {
		logger().log(ex, __FILE__, __LINE__);
	};
}

string JablotronDeviceManager::dongleMatch(const HotplugEvent &e)
{
	const string &productID = e.properties()
		->getString("tty.ID_MODEL_ID", "");

	const string &vendorID = e.properties()
		->getString("tty.ID_VENDOR_ID", "");

	if (e.subsystem() == "tty") {
		if (vendorID == JABLOTRON_VENDOR_ID
				&& productID == JABLOTRON_PRODUCT_ID)
			return e.node();
	}

	return "";
}

/* Retransmission packet status is recommended to be done 3 times with
 * a minimum gap 200ms and 500ms maximum space (for an answer) - times
 * in the space, it is recommended to choose random.
 */
bool JablotronDeviceManager::transmitMessage(const string &msg, bool autoResult)
{
	string message;

	for (int i = 0; i < NUMBER_OF_RETRIES; i++) {
		if (!m_serial.write(msg)) {
			Thread::sleep(SLEEP_AFTER_FAILED.totalMilliseconds());
			continue;
		}

		if (!autoResult) {
			int i = 0;
			while(!getResponse() && i < NUMBER_OF_RETRIES)
				i++;
		}

		if (m_responseRcv.tryWait(RESPONSE_WAIT_MSEC)) {
			if (m_lastResponse == ERROR) {
				Thread::sleep(DELAY_BEETWEEN_CYCLES.totalMilliseconds());
				continue;
			}
			Thread::sleep(DELAY_AFTER_SET_SWITCH.totalMilliseconds());
			return true;
		}
		else {
			poco_error(logger(), "no response received");
			return false;
		}
	}

	Thread::sleep(DELAY_AFTER_SET_SWITCH.totalMilliseconds());
	return false;
}

bool JablotronDeviceManager::getResponse()
{
	string message;
	MessageType response = nextMessage(message);

	if (isResponse(response)) {
		m_lastResponse = response;
		m_responseRcv.set();
		return true;
	}

	return false;
}

bool JablotronDeviceManager::isResponse(MessageType type)
{
	return type == OK || type == ERROR;
}

bool JablotronDeviceManager::modifyValue(
			const DeviceID &deviceID, int value, bool autoResult)
{
	auto it = m_devices.find(deviceID);
	if (it == m_devices.end()) {
		throw NotFoundException(
			"device " + deviceID.toString()
			+ " not found");
	}

	if (it->second.cast<JablotronDeviceAC88>().isNull()) {
		throw InvalidArgumentException(
			"device " + it->first.toString()
			+ " is not Jablotron AC-88");
	}

	if (!deviceCache()->paired(deviceID)) {
		InvalidArgumentException(
			"device " + it->first.toString()
			+ " is not paired");
	}

	if (m_pgx.first == deviceID)
		m_pgx.second = value;
	else
		m_pgy.second = value;

	string modifyString = "\x1BTX ENROLL:0 PGX:" + to_string(m_pgx.second) +
		" PGY:" + to_string(m_pgy.second) + " ALARM:0 BEEP:FAST\n";

	return transmitMessage(modifyString, autoResult);
}

void JablotronDeviceManager::obtainLastValue()
{
	int value;
	for (auto &device : m_devices) {
		if (device.second.cast<JablotronDeviceAC88>().isNull())
			continue;

		if (!deviceCache()->paired(device.second->deviceID()))
			continue;

		try {
			value = lastValue(device.first, 0);
		}
		catch (const Exception &ex) {
			logger().log(ex, __FILE__, __LINE__);
			continue;
		}

		Mutex::ScopedLock guard(m_lock);

		if (!modifyValue(device.first, value, false)) {
			logger().warning(
				"last value on device: " + device.first.toString()
				+ " is not set",
				__FILE__, __LINE__);
		}
	}
}

AsyncWork<double>::Ptr JablotronDeviceManager::startSetValue(
		const DeviceID &id,
		const ModuleID &,
		const double value,
		const Timespan &timeout)
{
	Mutex::ScopedLock guard(m_lock, timeout.totalMilliseconds());

	if (!modifyValue(id, value)) {
		throw IllegalStateException(
			"failed set-value: " + id.toString());
	}

	auto work = BlockingAsyncWork<double>::instance();
	work->setResult(value);
	return work;
}
