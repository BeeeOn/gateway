#include <Poco/Clock.h>
#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/Message.h>
#include <Poco/NumberFormatter.h>

#include "zwave/ZWaveSerialProber.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

static const char SOF       = 0x01;
static const char REQUEST   = 0x00;
static const char RESPONSE  = 0x01;
static const char VERSION   = 0x15;
static const char ACK       = 0x06;
static const char NACK      = 0x15;

static const size_t HEADER_SIZE = 2;

ZWaveSerialProber::ZWaveSerialProber(SerialPort &port):
	m_port(port)
{
}

void ZWaveSerialProber::probe(const Timespan &timeout)
{
	const Clock started;

	if (timeout < 0)
		throw InvalidArgumentException("timeout must not be negative");

	if (!m_port.isOpen())
		m_port.open();

	m_port.flush();

	nack(timeout - started.elapsed());
	const auto &ver = version(timeout - started.elapsed());

	logger().information("detected " + ver);
}

string ZWaveSerialProber::buildMessage(const vector<uint8_t> payload) const
{
	poco_assert(payload.size() <= 254);

	uint8_t size = payload.size() + 1;
	string message = {SOF, static_cast<char>(size)};

	uint8_t csum = 0xff;
	csum ^= static_cast<uint8_t>(size);

	for (const auto &b : payload) {
		csum ^= b;
		message += static_cast<char>(b);
	}

	message += static_cast<char>(csum);
	return message;
}

void ZWaveSerialProber::writeAll(const string &s, const Timespan &timeout)
{
	const Clock started;
	size_t total = 0;

	if (logger().trace()) {
		logger().dump(
			"write " + to_string(s.size()) + " bytes",
			s.data(),
			s.size(),
			Message::PRIO_TRACE);
	}
	else if (logger().debug()) {
		logger().debug(
			"write " + to_string(s.size()) + " bytes",
			__FILE__, __LINE__);
	}

	while (total < s.size()) {
		size_t wlen = m_port.write(s.data() + total, s.size() - total);

		if (wlen == 0) {
			if (!started.isElapsed(timeout.totalMicroseconds()))
				continue;

			throw TimeoutException("could not write to serial port");
		}

		total += wlen;
	}
}

void ZWaveSerialProber::checkTimeout(const Poco::Timespan &timeout) const
{
	if (timeout < 0)
		throw TimeoutException("timeout exceeded during probe");
}

string ZWaveSerialProber::read(size_t max, const Timespan &timeout)
{
	string s;

	if (m_buffer.size() >= max) {
		const auto end = m_buffer.begin() + max;

		s = {m_buffer.begin(), end};
		m_buffer.erase(m_buffer.begin(), end);
	}
	else if (m_buffer.size() < max) {
		s = m_buffer;
		m_buffer.clear();
	}

	if (s.empty()) {
		s = m_port.read(timeout);

		if (s.size() > max) {
			m_buffer = {s.begin() + s.size(), s.end()};
			s.erase(s.begin() + s.size(), s.end());
		}
	}

	if (logger().trace()) {
		logger().dump(
			"read " + to_string(s.size()) + " bytes",
			s.data(),
			s.size(),
			Message::PRIO_TRACE);
	}
	else if (logger().debug()) {
		logger().debug(
			"read " + to_string(s.size()) + " bytes",
			__FILE__, __LINE__);
	}

	return s;
}

void ZWaveSerialProber::nack(const Timespan &timeout)
{
	if (logger().debug())
		logger().debug("sending NACK", __FILE__, __LINE__);

	checkTimeout(timeout);
	writeAll({NACK}, timeout);
}

void ZWaveSerialProber::writeAck(const Timespan &timeout)
{
	writeAll({ACK}, timeout);
}

void ZWaveSerialProber::readAck(const Timespan &timeout)
{
	const auto &ack = read(1, timeout);
	if (ack != string{ACK})
		throw ProtocolException("received unexpected data, expected ACK");
}

size_t ZWaveSerialProber::decodeHeader(const string &message) const
{
	if (message.size() < HEADER_SIZE)
		throw ProtocolException("too short message, at least 2 bytes required");

	if (message[0] != SOF)
		throw ProtocolException("unexpected response, missing SOF byte");

	return static_cast<uint8_t>(message[1]);
}

void ZWaveSerialProber::writeRequest(const char id, const Timespan &timeout)
{
	writeAll(buildMessage({REQUEST, static_cast<uint8_t>(id)}), timeout);
}

string ZWaveSerialProber::readResponse(const Timespan &timeout)
{
	const Clock started;

	string response;

	while (response.size() < HEADER_SIZE) {
		response += read(
			HEADER_SIZE - response.size(),
			timeout - started.elapsed());
	}

	size_t size = decodeHeader(response);

	while (response.size() < (HEADER_SIZE + size)) {
		response += read(
			size - (response.size() - HEADER_SIZE),
			timeout - started.elapsed());
	}

	if (response[2] != RESPONSE)
		throw ProtocolException("unexpected data, expected RESPONSE byte");

	const string payload = {response.begin() + HEADER_SIZE, response.end()};
	const uint8_t paysum = static_cast<uint8_t>(payload[size - 1]);

	uint8_t csum = 0xff;
	csum ^= static_cast<uint8_t>(size);

	for (size_t i = 0; i < size - 1; ++i)
		csum ^= static_cast<uint8_t>(payload[i]);

	if (paysum != csum) {
		throw ProtocolException(
			"bad checksum " + NumberFormatter::formatHex(csum, 2)
			+ ", expected: " + NumberFormatter::formatHex(paysum, 2));
	}

	return {payload.begin() + 1, payload.end()};
}

string ZWaveSerialProber::version(const Timespan &timeout)
{
	if (logger().debug())
		logger().debug("probing version", __FILE__, __LINE__);

	checkTimeout(timeout);

	const Clock started;

	writeRequest(VERSION, timeout);
	readAck(timeout - started.elapsed());
	writeAck(timeout - started.elapsed());

	const auto &response = readResponse(timeout - started.elapsed());
	writeAck(timeout - started.elapsed());

	if (response[0] != VERSION)
		throw ProtocolException("unexpected data, expected VERSION response");

	const auto end = response.find(static_cast<char>(0x00));
	if (end == string::npos)
		throw ProtocolException("missing zero byte in VERSION response");

	const string libver = {response.begin() + 1, response.begin() + end};
	const int type = response[response.size() - 2];

	return libver + " (type: " + to_string(type) + ")";
}

void ZWaveSerialProber::setupPort(SerialPort &port)
{
	port.setBaudRate(115200);
	port.setParity(SerialPort::PARITY_NONE);
	port.setStopBits(SerialPort::STOPBITS_1);
}
