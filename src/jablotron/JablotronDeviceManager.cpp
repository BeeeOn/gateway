#include <Poco/RegularExpression.h>
#include <Poco/Thread.h>
#include <Poco/StringTokenizer.h>

#include "commands/NewDeviceCommand.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "io/AutoClose.h"
#include "jablotron/JablotronDeviceManager.h"
#include "udev/UDevEvent.h"
#include "util/LambdaTimerTask.h"

BEEEON_OBJECT_BEGIN(BeeeOn, JablotronDeviceManager)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(UDevListener)
BEEEON_OBJECT_REF("distributor", &JablotronDeviceManager::setDistributor)
BEEEON_OBJECT_REF("commandDispatcher", &JablotronDeviceManager::setCommandDispatcher)
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
static const Timespan READ_TIMEOUT = 1 * Timespan::SECONDS;
static const Timespan SLEEP_AFTER_FAILED = 1 * Timespan::SECONDS;
static const Timespan DELAY_AFTER_SET_SWITCH = 1 * Timespan::SECONDS;
static const Timespan DELAY_BEETWEEN_CYCLES = 300 * Timespan::MILLISECONDS;

JablotronDeviceManager::JablotronDeviceManager():
	DongleDeviceManager(DevicePrefix::PREFIX_JABLOTRON),
	m_lastResponse(NONE),
	m_isListen(false)
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

	try {
		jablotronProcess();
	}
	catch (const IOException &ex) {
		// waiting so that UDevEvent can arrive after exception on serial port
		Thread::sleep(SLEEP_AFTER_FAILED.totalMilliseconds());
		ex.rethrow();
	}
}

void JablotronDeviceManager::stop()
{
	DongleDeviceManager::stop();
	m_listenTimer.cancel(true);
}

void JablotronDeviceManager::jablotronProcess()
{
	string message;

	setupDongleDevices();
	loadDeviceList();

	while (!m_stop) {
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

		if (message.empty()) {
			logger().debug("empty message",
				__FILE__, __LINE__);
		}

		DeviceID id = JablotronDevice::buildID(extractSerialNumber(message));
		Mutex::ScopedLock guard(m_lock);

		auto it = m_devices.find(id);
		if (!it->second.isNull() && it->second->paired()) {
			shipMessage(message, it->second);
		}
		else if (m_isListen) {
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

bool JablotronDeviceManager::accept(const Command::Ptr cmd)
{
	if (cmd->is<DeviceSetValueCommand>())
		return true;
	else if (cmd->is<GatewayListenCommand>())
		return true;
	else if (cmd->is<DeviceUnpairCommand>()) {
		auto it = m_devices.find(cmd->cast<DeviceUnpairCommand>().deviceID());
		return it != m_devices.end();
	}
	else if (cmd->is<DeviceAcceptCommand>()) {
		return cmd->cast<DeviceAcceptCommand>().deviceID().prefix() == m_prefix;
	}

	return false;
}

void JablotronDeviceManager::handle(Command::Ptr cmd, Answer::Ptr answer)
{
	if (cmd->is<DeviceSetValueCommand>())
		doSetValue(cmd.cast<DeviceSetValueCommand>(), answer);
	else if (cmd->is<GatewayListenCommand>())
		doListenCommand(cmd.cast<GatewayListenCommand>(), answer);
	else if (cmd->is<DeviceUnpairCommand>())
		doUnpairCommand(cmd.cast<DeviceUnpairCommand>(), answer);
	else if (cmd->is<DeviceAcceptCommand>())
		doDeviceAcceptCommand(cmd.cast<DeviceAcceptCommand>(), answer);
	else
		throw IllegalStateException("received unaccepted command");
}

void JablotronDeviceManager::doDeviceAcceptCommand(
	const DeviceAcceptCommand::Ptr cmd, const Answer::Ptr answer)
{
	Mutex::ScopedLock guard(m_lock);
	Result::Ptr result = new Result(answer);

	auto it = m_devices.find(cmd->deviceID());
	if (it == m_devices.end()) {
		logger().error(
			"unknown " + cmd->deviceID().toString()
			+ " device", __FILE__, __LINE__);

		result->setStatus(Result::FAILED);
	} else {
		it->second->setPaired(true);
		result->setStatus(Result::SUCCESS);
	}
}

void JablotronDeviceManager::doUnpairCommand(
	const DeviceUnpairCommand::Ptr cmd, const Answer::Ptr answer)
{
	Result::Ptr result = new Result(answer);

	Mutex::ScopedLock guard(m_lock);
	auto it = m_devices.find(cmd->deviceID());

	if (it != m_devices.end()) {
		it->second = nullptr;
	}
	else {
		logger().warning(
			"attempt to unpair unknown device: "
			+ cmd->deviceID().toString());
	}

	result->setStatus(Result::SUCCESS);
}

void JablotronDeviceManager::doListenCommand(
	const GatewayListenCommand::Ptr cmd, const Answer::Ptr answer)
{
	Result::Ptr result = new Result(answer);
	result->setStatus(Result::SUCCESS);

	if (m_isListen)
		return;

	m_isListen = true;

	LambdaTimerTask::Ptr task(new LambdaTimerTask([=]()
	{
		logger().debug("listen is done", __FILE__, __LINE__);
		m_isListen = false;

		Mutex::ScopedLock guard(m_lock);
		for (auto &device : m_devices) {
			if (device.second.isNull())
				continue;

			if (!device.second->paired())
				device.second = nullptr;
		}
	}));

	m_listenTimer.schedule(
		task,
		Timestamp() + cmd->duration());
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

		it->second->setPaired(false);
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

			it->second->setPaired(true);
		}
	}
}

JablotronDeviceManager::MessageType JablotronDeviceManager::nextMessage(
			string &message)
{
	// find message between newline
	// message example: \nAC-88 RELAY:1\n
	const RegularExpression re("(?!\\n)[^\\n]*(?=\\n)");

	while (!re.extract(m_buffer, message) && !m_stop) {
		try {
			m_buffer += m_serial.read(READ_TIMEOUT);
		}
		catch (const TimeoutException &ex) {
			// avoid frozen state, allow to test m_stop time after time
			continue;
		}
	}

	// erase matched message from buffer + 2x newline
	m_buffer.erase(0, message.size() + 2);
	logger().debug("received data: " + message, __FILE__, __LINE__);

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
			logger().debug(
				"empty slot",
				__FILE__, __LINE__);
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

string JablotronDeviceManager::dongleMatch(const UDevEvent &e)
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

		Thread::sleep(DELAY_BEETWEEN_CYCLES.totalMilliseconds());
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

void JablotronDeviceManager::doSetValue(
	DeviceSetValueCommand::Ptr cmd, Answer::Ptr answer)
{
	Result::Ptr result = new Result(answer);
	result->setStatus(Result::FAILED);
}
