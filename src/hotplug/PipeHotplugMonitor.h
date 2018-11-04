#pragma once

#include <list>
#include <set>
#include <string>

#include <Poco/AtomicCounter.h>
#include <Poco/Event.h>
#include <Poco/Timespan.h>

#include "loop/StoppableRunnable.h"
#include "hotplug/AbstractHotplugMonitor.h"

namespace BeeeOn {

class FdInputStream;

/**
 * The PipeHotplugMonitor can be used to detect device hotplug
 * events independently on the underlying system. It is not
 * bound to any library like udev. It just waits on a given
 * file (named pipe) and reads events.
 *
 * An event recognized by the PipeHotplugMonitor is a sequence
 * of lines in form <code>KEY=VALUE</code>. Each event must be finished by
 * an empty line or EOF. There is a set of standard keys (matching
 * the HotplugEvent contents). The unrecognized keys are treated as
 * device properties. Each event must contain key ACTION defining
 * one of: add, remove, change, move.
 *
 * Example event:
 *
 * <pre>
 * ACTION=add<LF>
 * SUBSYSTEM=tty<LF>
 * NODE=/dev/ttyUSB0<LF>
 * DRIVER=serial_ftdi<LF>
 * <LF|EOF>
 * </pre>
 */
class PipeHotplugMonitor :
		public StoppableRunnable,
		public AbstractHotplugMonitor {
public:
	PipeHotplugMonitor();
	~PipeHotplugMonitor();

	void run() override;
	void stop() override;

	/**
	 * Set path to the pipe providing hotplug events.
	 * If the pipe does not exist, the PipeHotplugMonitor waits
	 * until it is created.
	 */
	void setPipePath(const std::string &path);

	/**
	 * Poll timeout determines how long to block while polling for
	 * new events. Negative value leads to blocking mode. A positive
	 * value leads to time-limited blocking and 0 denotes non-blocking.
	 * The timeout is expected at least in milliseconds.
	 */
	void setPollTimeout(const Poco::Timespan &timeout);

protected:
	/**
	 * Open the pipe for receiving hotplug events.
	 */
	int openPipe();

	/**
	 * Poll and read new events from the input.
	 */
	void pollForEvents(FdInputStream &input);

	/**
	 * Read a single hotplug event from the input.
	 * @return false when EOF was reached
	 */
	bool processEvent(FdInputStream &input);

	/**
	 * Parse a single line and break it to key and value.
	 * @return false when the line is malformed
	 */
	bool parseLine(const std::string &line, std::string &key, std::string &value) const;

	/**
	 * Interpret the key-value pair in the context of the HotplugEvent.
	 * Set either one of its members or extend properties.
	 */
	void fillEvent(HotplugEvent &event, const std::string &key, const std::string &value) const;

	/**
	 * Skip all lines until an empty line or EOF is reached.
	 * @return false when EOF was reached
	 */
	bool skipEvent(FdInputStream &input) const;
	void fireEvent(const HotplugEvent &event, const std::string &action);

private:
	std::string m_pipePath;
	Poco::Event m_waitPipe;
	Poco::Timespan m_pollTimeout;
	Poco::AtomicCounter m_stop;
};

}
