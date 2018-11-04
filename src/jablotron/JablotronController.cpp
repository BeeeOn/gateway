#include <Poco/Clock.h>
#include <Poco/Logger.h>
#include <Poco/Message.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>

#include "di/Injectable.h"
#include "jablotron/JablotronController.h"
#include "util/UnsafePtr.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

static const string CMD_BEGIN = "\x1B";
static const string CMD_END   = "\n";

static const string CMD_VERSION   = "WHO AM I?";

static string CMD_READ_SLOT(unsigned int i)
{
	return "GET SLOT:" + NumberFormatter::format0(i, 2);
}

static string CMD_SET_SLOT(unsigned int i, uint32_t addr)
{
	return "SET SLOT:" + NumberFormatter::format0(i, 2)
		+ " [" + NumberFormatter::format0(addr, 8) + "]";
}

static string CMD_CLEAR_SLOT(unsigned int i)
{
	return "SET SLOT:" + NumberFormatter::format0(i, 2)
		+ " [--------]";
}

static string CMD_ERASE_SLOTS = "ERASE ALL SLOTS";

static string CMD_TX(
		bool enroll,
		bool x = false,
		bool y = false,
		bool alarm = false,
		JablotronController::Beep beep = JablotronController::BEEP_NONE)
{
	const string en = enroll? "1" : "0";
	const string vx = x ? "1" : "0";
	const string vy = y ? "1" : "0";
	const string al = alarm? "1" : "0";

	string bp = "NONE";
	if (beep == JablotronController::BEEP_FAST)
		bp = "FAST";
	else if (beep == JablotronController::BEEP_SLOW)
		bp = "SLOW";

	return "TX ENROLL:" + en
		+ " PGX:" + vx
		+ " PGY:" + vy
		+ " ALARM:" + al
		+ " BEEP:" + bp;
}

JablotronController::JablotronController():
	m_maxProbeAttempts(5),
	m_probeTimeout(100 * Timespan::MILLISECONDS),
	m_ioJoinTimeout(2 * Timespan::SECONDS),
	m_ioReadTimeout(500 * Timespan::MILLISECONDS),
	m_ioErrorSleep(2 * Timespan::SECONDS),
	m_ioLoop(*this, &JablotronController::ioLoop)
{
}

void JablotronController::setMaxProbeAttempts(int count)
{
	if (count < 1)
		throw InvalidArgumentException("maxProbeAttempts must be at least 1");

	m_maxProbeAttempts = count;
}

void JablotronController::setProbeTimeout(const Timespan &timeout)
{
	if (timeout < 0)
		throw InvalidArgumentException("probeTimeout must not be negative");

	m_probeTimeout = timeout;
}

void JablotronController::setIOJoinTimeout(const Timespan &timeout)
{
	if (timeout < 0)
		throw InvalidArgumentException("ioJoinTimeout must not be negative");

	m_ioJoinTimeout = timeout;
}

void JablotronController::setIOReadTimeout(const Timespan &timeout)
{
	if (timeout < 0)
		m_ioReadTimeout = -1;
	else
		m_ioReadTimeout = timeout;
}

void JablotronController::setIOErrorSleep(const Timespan &delay)
{
	if (delay < 0)
		throw InvalidArgumentException("ioErrorSleep must not be negative");

	m_ioErrorSleep = delay;
}

void JablotronController::probe(const string &dev)
{
	FastMutex::ScopedLock guard(m_lock);

	if (!m_ioThread.isNull()) {
		logger().information("I/O thread is already running, ignoring " + dev);
		return;
	}

	try {
		probePort(dev);
	}
	BEEEON_CATCH_CHAIN_ACTION(logger(),
		m_port.close())

	startIO();
}

void JablotronController::release(const string &dev)
{
	FastMutex::ScopedLock guard(m_lock);

	m_requestEvent.set();
	m_pollEvent.set();

	stopIO(dev);
}

void JablotronController::dispose()
{
	FastMutex::ScopedLock guard(m_lock);

	m_requestEvent.set();
	m_pollEvent.set();

	stopIO(m_port.devicePath());
}

void JablotronController::startIO()
{
	m_ioThread = new Thread;
	m_joiner = new Joiner(*m_ioThread);

	while (!m_responses.empty())
		m_responses.pop();
	while (!m_reports.empty())
		m_reports.pop();

	m_requestEvent.reset();
	m_pollEvent.reset();

	m_ioThread->start(m_ioLoop);
}

void JablotronController::stopIO(const string &dev)
{
	if (m_ioThread.isNull())
		return;

	if (m_port.devicePath() != dev)
		return;

	logger().information("stopping I/O thread");

	m_stopControl.requestStop();

	if (!m_joiner->tryJoin(m_ioJoinTimeout))
		logger().critical("timeout while joining I/O thread", __FILE__, __LINE__);

	m_ioThread = nullptr;
	m_joiner = nullptr;
}

Nullable<uint32_t> JablotronController::readSlot(
		unsigned int i,
		const Timespan &timeout)
{
	static const RegularExpression pattern("^SLOT:([0-9][0-9]) \\[([-0-9]{8})\\]$");

	const auto data = command(CMD_READ_SLOT(i), timeout);
	RegularExpression::MatchVec m;

	if (pattern.match(data, 0, m)) {
		const auto addr = data.substr(m[2].offset, m[2].length);
		if (addr == "--------") {
			static const Nullable<uint32_t> null;
			return null;
		}

		unsigned int slot = NumberParser::parse(data.substr(m[1].offset, m[1].length));

		if (logger().debug()) {
			logger().debug(
				"slot " + to_string(slot)
				+ " has address " + addr,
				__FILE__, __LINE__);
		}

		if (slot != i) {
			throw IllegalStateException(
				"received result for slot " + to_string(slot)
				+ " but requested slot " + to_string(i));
		}

		return NumberParser::parseUnsigned(addr);
	}

	throw IllegalStateException("expected slot status but got: " + data);
}

void JablotronController::registerSlot(
		unsigned int i,
		uint32_t address,
		const Timespan &timeout)
{
	handleOkError(command(CMD_SET_SLOT(i, address), timeout));
}

void JablotronController::unregisterSlot(
		unsigned int i,
		const Timespan &timeout)
{
	handleOkError(command(CMD_CLEAR_SLOT(i), timeout));
}

void JablotronController::eraseSlots(const Timespan &timeout)
{
	handleOkError(command(CMD_ERASE_SLOTS, timeout));
}

void JablotronController::sendTX(
		bool x,
		bool y,
		bool alarm,
		Beep beep,
		const Timespan &timeout)
{
	handleOkError(command(CMD_TX(false, x, y, alarm, beep), timeout));
}

void JablotronController::sendEnroll(const Timespan &timeout)
{
	handleOkError(command(CMD_TX(true), timeout));
}

void JablotronController::handleOkError(const string &response) const
{
	if (response == "OK")
		return;

	if (response == "ERROR")
		throw ProtocolException("received result ERROR");

	throw IllegalStateException("received result " + response);
}

string JablotronController::command(const string &request, const Timespan &timeout)
{
	const Clock started;

	FastMutex::ScopedLock guard(m_requestLock);
	ScopedLockWithUnlock<FastMutex> tmpGuard(m_lock);

	if (!m_responses.empty()) {
		logger().warning("responses in queue before issuing a command: "
			+ to_string(m_responses.size()));
	}

	while (!m_responses.empty())
		m_responses.pop();

	writePort(CMD_BEGIN + request + CMD_END);
	tmpGuard.unlock();

	while (!m_stopControl.shouldStop()) {
		ScopedLockWithUnlock<FastMutex> tmp2guard(m_lock);

		if (!m_responses.empty())
			break;

		tmp2guard.unlock();

		Timespan remaining = timeout - started.elapsed();
		if (timeout < 0) {
			m_requestEvent.wait();
		}
		else if (remaining < 1 * Timespan::MILLISECONDS) {
			remaining = 1 * Timespan::MILLISECONDS;
			m_requestEvent.wait(remaining.totalMilliseconds());
		}
		else {
			m_requestEvent.wait(remaining.totalMilliseconds());
		}
	}

	return popResponse();
}

string JablotronController::popResponse()
{
	FastMutex::ScopedLock guard(m_lock);

	if (m_responses.empty())
		throw IllegalStateException("no response in the queue");

	const string last = m_responses.front();
	while (!m_responses.empty())
		m_responses.pop();

	return last;
}

JablotronReport JablotronController::pollReport(
	const Timespan &timeout)
{
	const auto report = popReport();
	if (report)
		return report;

	Timespan remaining = timeout;

	if (remaining < 0) {
		m_pollEvent.wait();
	}
	else if (remaining < 1 * Timespan::MILLISECONDS) {
		remaining = 1 * Timespan::MILLISECONDS;
		m_pollEvent.tryWait(remaining.totalMilliseconds());
	}
	else {
		m_pollEvent.tryWait(remaining.totalMilliseconds());
	}

	return popReport();
}

JablotronReport JablotronController::popReport()
{
	FastMutex::ScopedLock guard(m_lock);

	if (m_reports.empty()) {
		if (logger().debug())
			logger().debug("no report to pop", __FILE__, __LINE__);

		return JablotronReport::invalid();
	}

	const auto report = m_reports.front();

	if (logger().debug())
		logger().debug("pop report " + report.toString(), __FILE__, __LINE__);

	m_reports.pop();
	return report;
}

void JablotronController::processMessage(const string &message)
{
	static const RegularExpression report("\\[([0-9]{8})\\] ([^ ]+) ([^\\n]+)");

	RegularExpression::MatchVec m;

	if (report.match(message, 0, m)) {
		const auto address = NumberParser::parseUnsigned(
			message.substr(m[1].offset, m[1].length));
		const auto product = message.substr(m[2].offset, m[2].length);
		const auto data = message.substr(m[3].offset, m[3].length);

		FastMutex::ScopedLock guard(m_lock);

		const JablotronReport report = {address, product, data};

		if (logger().debug()) {
			logger().debug(
				"received report " + report.toString(),
				__FILE__, __LINE__);
		}

		m_reports.emplace(report);
		m_pollEvent.set();
	}
	else {
		FastMutex::ScopedLock guard(m_lock);

		if (logger().debug()) {
			logger().debug(
				"received response of size " + to_string(message.size()),
				__FILE__, __LINE__);
		}

		m_responses.emplace(message);
		m_requestEvent.set();
	}
}

void JablotronController::readAndProcess()
{
	static const RegularExpression pattern("\\n([^\\n]+)\\n");

	string buffer;
	size_t offset = 0;
	RegularExpression::MatchVec m;

	while (!pattern.match(buffer, 0, m)) {
		buffer += readPort(m_ioReadTimeout);
		m.clear();
	}

	do {
		try {
			processMessage(buffer.substr(m[1].offset, m[1].length));
		}
		BEEEON_CATCH_CHAIN(logger())

		offset += m[0].length;
		m.clear();
	}
	while (pattern.match(buffer, offset, m));
}

void JablotronController::ioLoop()
{
	UnsafePtr<Thread>(Thread::current())->setName("io-" + m_port.devicePath());

	logger().information("starting I/O thread");

	StopControl::Run run(m_stopControl);

	while (run) {
		try {
			readAndProcess();
		}
		catch (const IOException &e) {
			logException(logger(), Message::PRIO_ERROR, e, __FILE__, __LINE__);
			run.waitStoppable(m_ioErrorSleep);
			continue;
		}
		catch (const TimeoutException &) {
			continue;
		}
		BEEEON_CATCH_CHAIN(logger())
	}

	m_port.close();
	m_port.setDevicePath("");

	logger().information("I/O thread has finished");
}

void JablotronController::probePort(const string &dev)
{
	m_port.setBaudRate(57600);
	m_port.setStopBits(SerialPort::StopBits::STOPBITS_1);
	m_port.setParity(SerialPort::Parity::PARITY_NONE);
	m_port.setDataBits(SerialPort::DataBits::DATABITS_8);
	m_port.setDevicePath(dev);

	logger().information("probing port " + dev);

	m_port.open();
	m_port.flush();

	string buffer;

	try {
		// try to read and drop welcome message
		buffer = readPort(m_probeTimeout);
	}
	catch (const TimeoutException &) {
	}

	writePort(CMD_BEGIN + CMD_VERSION + CMD_END);

	for (size_t i = 0; i < m_maxProbeAttempts; ++i) {
		try {
			buffer += readPort(m_probeTimeout);
		}
		catch (const TimeoutException &) {
			continue;
		}

		if (receivedVersion(buffer))
			return;
	}

	throw TimeoutException("probe failed, version response was not received");
}

bool JablotronController::receivedVersion(const string &response)
{
	static const RegularExpression pattern("\\n([A-Z ]+V[0-9]\\.[0-9])( [A-Z]+)?\\n");
	RegularExpression::MatchVec m;
	size_t offset = 0;

	while (pattern.match(response, offset, m)) {
		offset += m[0].length;
		const auto message = response.substr(m[1].offset, m[1].length);

		logger().notice("detected dongle " + message);
		return true;
	}

	return false;
}

void JablotronController::writePort(const string &request)
{
	if (logger().trace()) {
		logger().dump(
			"writing to port " + m_port.devicePath() + " "
			+ to_string(request.size()) + " B",
			request.data(),
			request.size(),
			Message::PRIO_TRACE);
	}
	else if (logger().debug()) {
		logger().debug(
			"writing to port " + m_port.devicePath() + " "
			+ to_string(request.size()) + " B",
			__FILE__, __LINE__);
	}

	m_port.write(request);
}

string JablotronController::readPort(const Timespan &timeout)
{
	const auto data = m_port.read(timeout);

	if (data.empty())
		return "";

	if (logger().trace()) {
		logger().dump(
			"reading from port " + m_port.devicePath() + " "
			+ to_string(data.size()) + " B",
			data.data(),
			data.size(),
			Message::PRIO_TRACE);
	}
	else if (logger().debug()) {
		logger().debug(
			"reading from port " + m_port.devicePath() + " "
			+ to_string(data.size()) + " B",
			__FILE__, __LINE__);
	}

	return data;
}
