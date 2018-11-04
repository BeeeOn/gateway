#pragma once

#include <queue>
#include <string>

#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/Nullable.h>
#include <Poco/RunnableAdapter.h>
#include <Poco/SharedPtr.h>
#include <Poco/Thread.h>
#include <Poco/Timespan.h>

#include "io/SerialPort.h"
#include "jablotron/JablotronReport.h"
#include "loop/StopControl.h"
#include "util/Joiner.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief JablotronController provides access to the Turris Dongle
 * that is connected via a serial port. The Turris Dongle must be
 * probed to start an internal I/O thread that handles incoming messages.
 */
class JablotronController : Loggable {
public:
	enum Beep {
		BEEP_NONE,
		BEEP_SLOW,
		BEEP_FAST,
	};

	JablotronController();

	/**
	 * @brief Configure number of attempts to initiate communication with the
	 * serial port connected to a Turris Dongle.
	 */
	void setMaxProbeAttempts(int count);

	/**
	 * @brief Configure timeout to wait for response from Turris Dongle while
	 * probing it.
	 */
	void setProbeTimeout(const Poco::Timespan &timeout);

	/**
	 * @brief Configure join timeout when waiting for the I/O thread to finish.
	 */
	void setIOJoinTimeout(const Poco::Timespan &timeout);

	/**
	 * @brief Configure timeout to use while reading from the serial port inside
	 * the I/O loop. The value can be negative but in such case, it might not be
	 * woken up properly on exit.
	 */
	void setIOReadTimeout(const Poco::Timespan &timeout);

	/**
	 * @brief Set time interval to sleep for when there is an I/O error. It reduces
	 * the number of logged error messages and avoids making the CPU overloaded in
	 * such case.
	 */
	void setIOErrorSleep(const Poco::Timespan &delay);

	/**
	 * @brief Probe the given serial port (e.g. "/dev/ttyUSB0") and if
	 * it proves to be a Jablotron control station, the internal I/O
	 * thread is started. Only 1 I/O thread can run at a time and only
	 * 1 central can be served by an instance of this class.
	 *
	 * @throws Poco::IOException in case of a communication failure (station is disconnected)
	 * @throws Poco::TimeoutException is probe takes too long time (wrong device?)
	 */
	void probe(const std::string &dev);

	/**
	 * @brief Release the serial port and stop the I/O thread
	 * if running. If the argument dev does not match the current
	 * serial port, nothing happens.
	 */
	void release(const std::string &dev);

	/**
	 * @brief Release the serial port and stop the I/O thread
	 * if running.
	 */
	void dispose();

	/**
	 * @brief Poll for reports coming from Jablotron sensors.
	 * If the timeout exceeds, an invalid report is returned.
	 * The timeout can be -1 to denote infinite waiting.
	 */
	JablotronReport pollReport(const Poco::Timespan &timeout);

	/**
	 * @brief Read address of the given slot.
	 * @returns address of the device at the requested slot
	 * or null if not set.
	 */
	Poco::Nullable<uint32_t> readSlot(
		unsigned int i,
		const Poco::Timespan &timeout);

	/**
	 * @brief Register the given slot with the given address.
	 * @throws Poco::ProtocolException when response is "ERROR"
	 * @throws Poco::IllegalStateException when response is not "OK" nor "ERROR"
	 */
	void registerSlot(
		unsigned int i,
		uint32_t address,
		const Poco::Timespan &timeout);

	/**
	 * @brief Unregister the given slot.
	 * @throws Poco::ProtocolException when response is "ERROR"
	 * @throws Poco::IllegalStateException when response is not "OK" nor "ERROR"
	 */
	void unregisterSlot(
		unsigned int i,
		const Poco::Timespan &timeout);

	/**
	 * @brief Unregister all slots at once.
	 * @throws Poco::ProtocolException when response is "ERROR"
	 * @throws Poco::IllegalStateException when response is not "OK" nor "ERROR"
	 */
	void eraseSlots(const Poco::Timespan &timeout);

	/**
	 * @brief Send status packet with PGX and PGY set to either 1 or 0
	 * accodring to the given values. Alarm is disabled and beeping set
	 * to <code>FAST</code>.
	 * @throws Poco::ProtocolException when response is "ERROR"
	 * @throws Poco::IllegalStateException when response is not "OK" nor "ERROR"
	 */
	void sendTX(
		bool x,
		bool y,
		bool alarm,
		Beep beep,		
		const Poco::Timespan &timeout);

	/**
	 * @brief Send enroll packet with PGX, PGY and ALARM as 0 and no beeping.
	 * @throws Poco::ProtocolException when response is "ERROR"
	 * @throws Poco::IllegalStateException when response is not "OK" nor "ERROR"
	 */
	void sendEnroll(const Poco::Timespan &timeout);

protected:
	/**
	 * @brief Check whether the response is "OK" or "ERROR".
	 *
	 * @throws Poco::ProtocolException when response is "ERROR"
	 * @throws Poco::IllegalStateException when response is not "OK" nor "ERROR"
	 */
	void handleOkError(const std::string &response) const;

	/**
	 * @brief Issue a command and return its result unless the timeout exceeds.
	 * @throws Poco::TimeoutException - no response on time
	 */
	std::string command(const std::string &request,
		const Poco::Timespan &timeout);

	/**
	 * @brief Start the I/O thread without any checks and
	 * clear any thread-related status.
	 */
	void startIO();

	/**
	 * @brief Stop the I/O thread is any and if the argument
	 * dev matches the currently used serial port.
	 * The call blocks until the I/O threads finishes or
	 * the ioJoinTimeout exceeds.
	 */
	void stopIO(const std::string &dev);

	/**
	 * @brief Probe the given port and test whether it is
	 * an appropriate Turris Dongle. Any obsolete messages in
	 * the serial port's buffer (that are invalid) are discarded.
	 */
	void probePort(const std::string &dev);

	/**
	 * @brief Parse the response to be the version string.
	 * @returns true if the version string was recognized
	 */
	bool receivedVersion(const std::string &response);

	/**
	 * @brief Pop the most recent received response. All
	 * older responses are dropped because only 1 command
	 * can be issued at a time. All pending older responses
	 * are dropped as they are probably some past unhandled
	 * responses.
	 */
	std::string popResponse();

	/**
	 * @brief Pop the oldest report from the associated queue.
	 * If the queue is empty, it returns an invalid report.
	 */
	JablotronReport popReport();

	/**
	 * @brief Read the serial port and process all received
	 * messages in order.
	 * @throws Poco::TimeoutException is ioReadTimeout exceeds
	 * while reading from the serial port
	 */
	void readAndProcess();

	/**
	 * @brief Process the given message. The call recognizes
	 * 2 kinds of messages: reports from sensors and other
	 * (unknown) messages. The reports are appended into the
	 * m_reports queue and the m_pollEvent is signalzed.
	 * All non-report messages are appended into the m_responses
	 * queue and the m_requestEvent is signalized.
	 */
	void processMessage(const std::string &message);

	/**
	 * @brief The entry into the I/O thread loop. It periodically
	 * calls readAndProcess() and checks whether a stop is requested.
	 * Any time an IOException is caught, the loop sleeps for
	 * ioErrorSleep and tries again. Timeouts from serial port
	 * are ignored.
	 */
	void ioLoop();

	void writePort(const std::string &request);
	std::string readPort(const Poco::Timespan &timeout);

private:
	SerialPort m_port;
	std::queue<std::string> m_responses;
	Poco::Event m_requestEvent;
	std::queue<JablotronReport> m_reports;
	Poco::Event m_pollEvent;

	size_t m_maxProbeAttempts;
	Poco::Timespan m_probeTimeout;
	Poco::Timespan m_ioJoinTimeout;
	Poco::Timespan m_ioReadTimeout;
	Poco::Timespan m_ioErrorSleep;

	Poco::SharedPtr<Poco::Thread> m_ioThread;
	Poco::SharedPtr<Joiner> m_joiner;
	Poco::RunnableAdapter<JablotronController> m_ioLoop;
	StopControl m_stopControl;

	Poco::FastMutex m_lock;
	Poco::FastMutex m_requestLock;
};

}
