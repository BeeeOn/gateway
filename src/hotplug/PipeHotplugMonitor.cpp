#include <cerrno>
#include <cstdio>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/String.h>
#include <Poco/StringTokenizer.h>

#include "di/Injectable.h"
#include "hotplug/HotplugEvent.h"
#include "hotplug/PipeHotplugMonitor.h"
#include "io/FdStream.h"

BEEEON_OBJECT_BEGIN(BeeeOn, PipeHotplugMonitor)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_PROPERTY("pipePath", &PipeHotplugMonitor::setPipePath)
BEEEON_OBJECT_PROPERTY("pollTimeout", &PipeHotplugMonitor::setPollTimeout)
BEEEON_OBJECT_PROPERTY("listeners", &PipeHotplugMonitor::registerListener)
BEEEON_OBJECT_END(BeeeOn, PipeHotplugMonitor)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

static const Timespan WAIT_PIPE_TIMEOUT = 100 * Timespan::MILLISECONDS;
static const string DEFAULT_PATH = "/var/run/beeeon-gateway.hotplug";

PipeHotplugMonitor::PipeHotplugMonitor():
	m_pipePath(DEFAULT_PATH),
	m_pollTimeout(WAIT_PIPE_TIMEOUT),
	m_stop(false)
{
}

PipeHotplugMonitor::~PipeHotplugMonitor()
{
}

bool PipeHotplugMonitor::parseLine(const string &line, string &key, string &value) const
{
	StringTokenizer pair(line, "=",
		StringTokenizer::TOK_TRIM | StringTokenizer::TOK_IGNORE_EMPTY);

	if (pair.count() != 2)
		return false;

	key = pair[0];
	value = pair[1];
	return true;
}

void PipeHotplugMonitor::fillEvent(HotplugEvent &event, const string &key, const string &value) const
{
	if (toLower(key) == "subsystem")
		event.setSubsystem(value);
	else if (toLower(key) == "name")
		event.setName(value);
	else if (toLower(key) == "node")
		event.setNode(value);
	else if (toLower(key) == "type")
		event.setType(value);
	else if (toLower(key) == "driver")
		event.setDriver(value);
	else
		event.properties()->setString(key, value);
}

bool PipeHotplugMonitor::skipEvent(FdInputStream &input) const
{
	string line;

	while (getline(input, line)) {
		if (m_stop)
			return false;

		// if empty line reached, try next event
		if (line.empty())
			return true;
	}

	// EOF reached
	return false;
}

bool PipeHotplugMonitor::processEvent(FdInputStream &input)
{
	HotplugEvent event;
	string action;
	string line;
	bool nothingParsable = true;

	while (getline(input, line)) {
		if (logger().trace()) {
			logger().trace("line: " + line,
			       __FILE__, __LINE__);
		}

		if (m_stop) {
			logger().debug("detected stop request",
					__FILE__, __LINE__);
			return false;
		}

		// end of event
		if (line.empty())
			break;

		string key;
		string value;

		if (!parseLine(line, key, value)) {
			logger().warning("invalid input line: " + line,
				 __FILE__, __LINE__);
			continue;
		}

		nothingParsable = false;

		if (toLower(key) == "action") {
			if (!action.empty()) {
				logger().warning(
					"duplicate entry action (was "
					+ action
					+ ") "
					+ value
					+ ", ignoring",
					__FILE__, __LINE__);

				return skipEvent(input);
			}

			action = value;
		}
		else {
			fillEvent(event, key, value);
		}
	}

	if (nothingParsable)
		return false;

	if (action.empty()) {
		logger().warning(
			"no action for event "
			+ event.toString()
			+ ", ignoring",
			__FILE__, __LINE__);

		return true;
	}

	logEvent(event, action);
	fireEvent(event, action);

	return true;
}

void PipeHotplugMonitor::fireEvent(const HotplugEvent &event, const string &action)
{
	if (toLower(action) == "add") {
		fireAddEvent(event);
	}
	else if (toLower(action) == "remove") {
		fireRemoveEvent(event);
	}
	else if (toLower(action) == "change") {
		fireChangeEvent(event);
	}
	else if (toLower(action) == "move") {
		fireMoveEvent(event);
	}
	else {
		logger().warning(
			"invalid action "
			+ action
			+ " for event "
			+ event.toString(),
			__FILE__, __LINE__
		);
	}
}

int PipeHotplugMonitor::openPipe()
{
	int fd = -1;

	while (!m_stop) {
		fd = ::open(m_pipePath.c_str(), O_RDONLY | O_NONBLOCK);

		if (fd >= 0)
			break;

		if (m_waitPipe.tryWait(WAIT_PIPE_TIMEOUT.totalMilliseconds()))
			break;
	}

	return fd;
}

void PipeHotplugMonitor::pollForEvents(FdInputStream &input)
{
	while (!m_stop && input.good()) {
		try {
			if (!input.poll(m_pollTimeout))
				continue;

			// we are readable, process as much events as possible
			while (!m_stop && processEvent(input))
				/* nothing */;
		}
		catch (const FileException &e) {
			if (logger().debug())
				logger().log(e, __FILE__, __LINE__);

			break; // poll failed, reopen pipe
		}
		catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
		}
		catch (const exception &e) {
			logger().critical(e.what(), __FILE__, __LINE__);
		}
		catch (...) {
			logger().critical("unknown failure",
					  __FILE__, __LINE__);
		}
	}
}

void PipeHotplugMonitor::run()
{
	logger().notice("starting hotplug monitoring");
	logger().information(
		"polling "
		+ m_pipePath
		+ " for hotplug events",
		__FILE__, __LINE__);

	while (!m_stop) {
		const int fd = openPipe();

		if (fd < 0)
			break;

		FdInputStream input(fd);
		input.setBlocking(false);

		logger().debug("pipe ready for polling",
				__FILE__, __LINE__);

		pollForEvents(input);
	}

	logger().notice("stopping hotplug monitoring");
	m_stop = false;
	m_waitPipe.reset();
}

void PipeHotplugMonitor::stop()
{
	m_stop = true;
	m_waitPipe.set();
}

void PipeHotplugMonitor::setPipePath(const string &pipePath)
{
	m_pipePath = pipePath;
}

void PipeHotplugMonitor::setPollTimeout(const Timespan &timeout)
{
	// 0 ... non-blocking
	// negative ... blocking
	// positive ... blocking with timeout

	if (timeout < 0) {
		m_pollTimeout = -1 * Timespan::MILLISECONDS;
	}
	else if (timeout == 0) {
		m_pollTimeout = 0;
	}
	else if (timeout < 1 * Timespan::MILLISECONDS) {
		throw InvalidArgumentException(
			"pollTimeout must be at least 1 ms");
	}
	else {
		m_pollTimeout = timeout;
	}
}
