#pragma once

#include <string>
#include <vector>

#include <Poco/Timespan.h>

#include "io/SerialPort.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief ZWaveSerialProber detects whether the device connected to
 * a serial port is a Z-Wave controller. We try to obtain its version
 * (and report it). If the version cannot be obtained an exception is
 * thrown.
 */
class ZWaveSerialProber : Loggable {
public:
	ZWaveSerialProber(SerialPort &port);

	/**
	 * @brief Probe the configured serial port and try to find a
	 * Z-Wave controller on the other side. We send NACK and then
	 * the version request.
	 *
	 * The whole probing process must fit into the given timeout.
	 * to prevent infinite waiting. The timeout must be a positive
	 * value.
	 *
	 * @throws Poco::InvalidArgumentException - when timeout is invalid
	 * @throws Poco::TimeoutException - when timeout exceeds while probing
	 * @throws Poco::ProtocolException - when the remote device gives unexpected results
	 */
	void probe(const Poco::Timespan &timeout);

	/**
	 * @brief Configure the given serial port to settings typical
	 * for Z-Wave controllers.
	 */
	static void setupPort(SerialPort &port);

protected:
	/**
	 * @brief Build a message from the given payload. The header with
	 * SOF and size are prepended and the checksum is computed and appended.
	 */
	std::string buildMessage(const std::vector<uint8_t> payload) const;

	/**
	 * @brief Check whether the given timeout is positive. Otherwise, we
	 * treate it as expired.
	 *
	 * @throws Poco::TimeoutException
	 */
	void checkTimeout(const Poco::Timespan &timeout) const;

	void writeAll(const std::string &s, const Poco::Timespan &timeout);
	std::string read(size_t max, const Poco::Timespan &timeout);

	void writeAck(const Poco::Timespan &timeout);
	void readAck(const Poco::Timespan &timeout);
	size_t decodeHeader(const std::string &message) const;
	void writeRequest(const char id, const Poco::Timespan &timeout);
	std::string readResponse(const Poco::Timespan &timeout);

	void nack(const Poco::Timespan &timeout);
	std::string version(const Poco::Timespan &timeout);

private:
	SerialPort &m_port;
	std::string m_buffer;
};

}
