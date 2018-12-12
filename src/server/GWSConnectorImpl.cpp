#include <Poco/Buffer.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/Message.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NObserver.h>
#include <Poco/Random.h>
#include <Poco/Thread.h>

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/NetException.h>
#include <Poco/Net/SocketReactor.h>

#include "di/Injectable.h"
#include "gwmessage/GWGatewayAccepted.h"
#include "gwmessage/GWGatewayRegister.h"
#include "server/GWSConnectorImpl.h"
#include "util/UnsafePtr.h"

BEEEON_OBJECT_BEGIN(BeeeOn, GWSConnectorImpl)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(GWSConnector)
BEEEON_OBJECT_PROPERTY("host", &GWSConnectorImpl::setHost)
BEEEON_OBJECT_PROPERTY("port", &GWSConnectorImpl::setPort)
BEEEON_OBJECT_PROPERTY("maxMessageSize", &GWSConnectorImpl::setMaxMessageSize)
BEEEON_OBJECT_PROPERTY("sslConfig", &GWSConnectorImpl::setSSLConfig)
BEEEON_OBJECT_PROPERTY("receiveTimeout", &GWSConnectorImpl::setReceiveTimeout)
BEEEON_OBJECT_PROPERTY("sendTimeout", &GWSConnectorImpl::setSendTimeout)
BEEEON_OBJECT_PROPERTY("reconnectDelay", &GWSConnectorImpl::setReconnectDelay)
BEEEON_OBJECT_PROPERTY("keepAliveTimeout", &GWSConnectorImpl::setKeepAliveTimeout)
BEEEON_OBJECT_PROPERTY("outputsCount", &GWSConnectorImpl::setOutputsCount)
BEEEON_OBJECT_PROPERTY("maxFailedReceives", &GWSConnectorImpl::setMaxFailedReceives)
BEEEON_OBJECT_PROPERTY("gatewayInfo", &GWSConnectorImpl::setGatewayInfo)
BEEEON_OBJECT_PROPERTY("priorityAssigner", &GWSConnectorImpl::setPriorityAssigner)
BEEEON_OBJECT_PROPERTY("listeners", &GWSConnectorImpl::addListener)
BEEEON_OBJECT_PROPERTY("eventsExecutor", &GWSConnectorImpl::setEventsExecutor)
BEEEON_OBJECT_HOOK("done", &GWSConnectorImpl::setupQueues)
BEEEON_OBJECT_HOOK("cleanup", &GWSConnectorImpl::clearListeners)
BEEEON_OBJECT_END(BeeeOn, GWSConnectorImpl)

using namespace std;
using namespace Poco;
using namespace Poco::Net;
using namespace BeeeOn;

GWSConnectorImpl::GWSConnectorImpl():
	m_host("127.0.0.1"),
	m_port(8850),
	m_maxMessageSize(4096),
	m_receiveTimeout(3 * Timespan::SECONDS),
	m_sendTimeout(1 * Timespan::SECONDS),
	m_reconnectDelay(5 * Timespan::SECONDS),
	m_keepAliveTimeout(30 * Timespan::SECONDS),
	m_receiveFailed(0)
{
}

void GWSConnectorImpl::setHost(const string &host)
{
	m_host = host;
}

void GWSConnectorImpl::setPort(int port)
{
	if (port < 0)
		throw InvalidArgumentException("port must not be negative");

	m_port = port;
}

void GWSConnectorImpl::setMaxMessageSize(int size)
{
	if (size <= 0)
		throw InvalidArgumentException("maxMessageSize must be positive");

	m_maxMessageSize = size;
}

void GWSConnectorImpl::setSSLConfig(SSLClient::Ptr config)
{
	m_sslConfig = config;
}

void GWSConnectorImpl::setReceiveTimeout(const Timespan &timeout)
{
	m_receiveTimeout = timeout;
}

void GWSConnectorImpl::setSendTimeout(const Timespan &timeout)
{
	m_sendTimeout = timeout;
}

void GWSConnectorImpl::setReconnectDelay(const Timespan &delay)
{
	if (delay < 0)
		throw InvalidArgumentException("reconnectDelay must not be negative");

	m_reconnectDelay = delay;
}

void GWSConnectorImpl::setKeepAliveTimeout(const Timespan &timeout)
{
	if (m_keepAliveTimeout >= 0 && m_keepAliveTimeout < 1 * Timespan::MILLISECONDS)
		throw InvalidArgumentException("keepAliveTimeout must be at least 1 ms");
	m_keepAliveTimeout = timeout;
}

void GWSConnectorImpl::setMaxFailedReceives(int count)
{
	if (count < 1)
		throw InvalidArgumentException("maxFailedReceives must be at least 1");

	m_maxFailedReceives = count;
}

void GWSConnectorImpl::setGatewayInfo(GatewayInfo::Ptr info)
{
	m_gatewayInfo = info;
}

void GWSConnectorImpl::run()
{
	StopControl::Run run(m_stopControl);
	const GWSListener::Address address = {m_host, m_port};

	UnsafePtr<Thread>(Thread::current())
		->setName("gws-main-" + address.toString());

	if (m_keepAliveTimeout < 0)
		logger().warning("keep-alive timeout is off", __FILE__, __LINE__);

	while (run) {
		SharedPtr<WebSocket> socket;

		try {
			socket = connect(address.host, address.port);
		}
		BEEEON_CATCH_CHAIN_ACTION(logger(),
			waitBeforeReconnect();
			continue)

		if (!run)
			break;

		try {
			performRegister(*socket);
		}
		BEEEON_CATCH_CHAIN_ACTION(logger(),
			waitBeforeReconnect();
			continue)

		fireEvent(address, &GWSListener::onConnected);

		SocketReactor reactor;
		NObserver<GWSConnectorImpl, ReadableNotification> observer(
			*this, &GWSConnectorImpl::onReadable);
		reactor.addEventHandler(*socket, observer);
		Thread reactorThread;
		reactorThread.setName("gws-read-" + address.toString());

		if (logger().debug())
			logger().debug("starting reactor thread...", __FILE__, __LINE__);

		m_receiveFailed = 0;

		reactorThread.start(reactor);
		outputLoop(run, *socket);

		if (logger().debug())
			logger().debug("stopping reactor thread...", __FILE__, __LINE__);

		reactor.removeEventHandler(*socket, observer);
		reactor.stop();
		reactorThread.join();

		fireEvent(address, &GWSListener::onDisconnected);

		if (run)
			waitBeforeReconnect();
	}
}

void GWSConnectorImpl::stop()
{
	m_stopControl.requestStop();
	m_outputsUpdated.set();
}

void GWSConnectorImpl::outputLoop(
		StopControl::Run &run,
		WebSocket &socket)
{
	while (run) {
		if (m_receiveFailed > m_maxFailedReceives)
			break;

		try {
			if (performOutput(socket))
				continue;

			if (waitOutputs())
				continue;

			checkPingTimeout();
			performPing(socket);
		}
		BEEEON_CATCH_CHAIN_ACTION(logger(),
			break)
	}
}

void GWSConnectorImpl::waitBeforeReconnect()
{
	logger().information(
		"reconnecting in " + DateTimeFormatter::format(m_reconnectDelay));

	m_stopControl.waitStoppable(m_reconnectDelay);
}

bool GWSConnectorImpl::waitOutputs()
{
	if (m_keepAliveTimeout < 0) {
		logger().debug(
			"output queue is empty for, sleeping...",
			__FILE__, __LINE__);

		m_outputsUpdated.wait();
		return true;
	}
	else {
		Timespan timeout = keepAliveRemaining();
		if (timeout < 0)
			return false;

		if (logger().debug()) {
			logger().debug(
				"output queue is empty for, sleeping with timeout "
				+ DateTimeFormatter::format(timeout),
				__FILE__, __LINE__);
		}

		if (m_outputsUpdated.tryWait(timeout.totalMilliseconds()))
			return true;

		return keepAliveRemaining() < 0;
	}
}

const Timespan GWSConnectorImpl::keepAliveRemaining() const
{
	FastMutex::ScopedLock guard(m_lock);

	poco_assert(m_keepAliveTimeout >= 0);

	Timespan timeout;

	if (m_lastActivity > m_lastPing)
		timeout = m_keepAliveTimeout - m_lastActivity.elapsed();
	else
		timeout = m_keepAliveTimeout - m_lastPing.elapsed();

	if (timeout < 0)
		return -1;

	if (timeout < 1 * Timespan::MILLISECONDS)
		return 1 * Timespan::MILLISECONDS;

	return timeout;
}

void GWSConnectorImpl::checkPingTimeout() const
{
	FastMutex::ScopedLock guard(m_lock);

	if (m_lastActivity > m_lastPing)
		return;

	if (!m_lastPing.isElapsed(m_keepAliveTimeout.totalMicroseconds()))
		return;

	throw TimeoutException(
		"server did not respond to ping on time");
}

SharedPtr<WebSocket> GWSConnectorImpl::connect(
		const string &host,
		const int port) const
{
	HTTPRequest request(HTTPRequest::HTTP_1_1);
	HTTPResponse response;

	SharedPtr<WebSocket> socket;

	logger().notice("connecting...", __FILE__, __LINE__);

	if (m_sslConfig.isNull()) {
		HTTPClientSession session(host, port);
		socket = new WebSocket(session, request, response);
	}
	else {
		SSLClient::Ptr config = m_sslConfig;
		HTTPSClientSession session(host, port, config->context());
		socket = new WebSocket(session, request, response);
	}

	if (m_receiveTimeout >= 0)
		socket->setReceiveTimeout(m_receiveTimeout);
	if (m_sendTimeout >= 0)
		socket->setSendTimeout(m_sendTimeout);

	if (logger().debug()) {
		logger().debug(
			"successfully connected",
			__FILE__, __LINE__);
	}

	return socket;
}

void GWSConnectorImpl::performRegister(WebSocket &socket) const
{
	logger().information(
		"registering gateway as "
		+ m_gatewayInfo->gatewayID().toString()
		+ " (" + socket.address().host().toString() + ")"
		+ " version " + m_gatewayInfo->version());

	GWGatewayRegister request;
	request.setID(GlobalID::random());
	request.setGatewayID(m_gatewayInfo->gatewayID());
	request.setIPAddress(socket.address().host());
	request.setVersion(m_gatewayInfo->version());

	sendMessage(socket, request);
	GWMessage::Ptr response = receiveMessage(socket);

	if (response.cast<GWGatewayAccepted>().isNull())
		throw ProtocolException("unexpected response: " + response->toBriefString());

	logger().notice("successfully registered", __FILE__, __LINE__);
}

bool GWSConnectorImpl::performOutput(WebSocket &socket)
{
	Mutex::ScopedLock guard(m_outputLock);

	const size_t i = selectOutput();
	if (!outputValid(i))
		return false; // nothing to output

	const GWMessage::Ptr message = peekOutput(i);

	try {
		fireEvent(message, &GWSListener::onTrySend);

		sendMessage(socket, *message);

		fireEvent(message, &GWSListener::onSent);
	}
	catch (const NetException &e) {
		e.rethrow();
	}
	BEEEON_CATCH_CHAIN(logger())

	popOutput(i);
	updateOutputs(i);

	return true;
}

void GWSConnectorImpl::performPing(WebSocket &socket)
{
	FastMutex::ScopedLock guard(m_lock);

	poco_assert(m_keepAliveTimeout >= 0);

	if (!m_lastPing.isElapsed(m_keepAliveTimeout.totalMicroseconds()))
		return;

	if (logger().debug())
		logger().debug("sending ping", __FILE__, __LINE__);

	Random r;
	const Timestamp now;
	string payload;

	payload += m_gatewayInfo->version();
	payload += " time-" + to_string(now.epochTime());
	payload += " nonce-" + to_string(r.next());

	const int flags = WebSocket::FRAME_OP_PING | WebSocket::FRAME_FLAG_FIN;
	sendFrame(socket, payload, flags);
	m_lastPing.update();
}

void GWSConnectorImpl::onReadable(const AutoPtr<ReadableNotification> &n)
{
	if (m_receiveFailed >= m_maxFailedReceives)
		return;

	WebSocket socket(n->socket());

	GWMessage::Ptr message;

	try {
		message = receiveMessage(socket);
	}
	BEEEON_CATCH_CHAIN_ACTION(logger(),
		n->source().stop();
		++m_receiveFailed;
		m_outputsUpdated.set())

	if (message.isNull())
		return;

	fireReceived(message);
}

void GWSConnectorImpl::sendMessage(
		WebSocket &socket,
		const GWMessage &message) const
{
	const auto &raw = message.toString();

	if (logger().debug()) {
		logger().debug(
			"sending message " + message.toBriefString(),
			__FILE__, __LINE__);
	}

	sendFrame(socket, raw, WebSocket::FRAME_TEXT);
}

void GWSConnectorImpl::sendFrame(
	WebSocket &socket,
	const string &payload,
	const int flags) const
{
	FastMutex::ScopedLock guard(m_sendLock);

	if (logger().trace()) {
		logger().dump(
			"sending frame of size " + to_string(payload.size())
			+ " (" + NumberFormatter::formatHex(flags, true) + ")",
			payload.data(),
			payload.size(),
			Message::PRIO_TRACE);
	}
	else if (logger().debug()) {
		logger().debug(
			"sending frame of size " + to_string(payload.size())
			+ " (" + NumberFormatter::formatHex(flags, true) + ")",
			__FILE__, __LINE__);
	}

	socket.sendFrame(payload.data(), payload.size(), flags);
}

int GWSConnectorImpl::receiveFrame(
		Poco::Net::WebSocket &socket,
		Buffer<char> &buffer,
		int &flags) const
{
	FastMutex::ScopedLock guard(m_receiveLock);
	return socket.receiveFrame(buffer.begin(), buffer.size(), flags);
}

GWMessage::Ptr GWSConnectorImpl::receiveMessage(WebSocket &socket) const
{
	Buffer<char> buffer(m_maxMessageSize);
	int flags;

	const int ret = receiveFrame(socket, buffer, flags);
	const int opcode = flags & WebSocket::FRAME_OP_BITMASK;

	if (ret < 0)
		throw ConnectionResetException("error while reading frame");

	if (ret == 0 && flags == 0)
		return nullptr;

	const string payload(buffer.begin(), ret);

	if (logger().trace()) {
		logger().dump(
			"received frame of size " + to_string(payload.size())
			+ " (" + NumberFormatter::formatHex(flags, true) + ")",
			payload.data(),
			payload.size(),
			Message::PRIO_TRACE);
	}
	else if (logger().debug()) {
		logger().debug(
			"received frame of size " + to_string(payload.size())
			+ " (" + NumberFormatter::formatHex(flags, true) + ")",
			__FILE__, __LINE__);
	}

	GWMessage::Ptr message;

	if (opcode == WebSocket::FRAME_OP_CLOSE) {
		throw ConnectionResetException("connection closed from server");
	}
	else if (opcode == WebSocket::FRAME_OP_PONG) {
		if (logger().debug())
			logger().debug("received pong frame", __FILE__, __LINE__);
	}
	else if (opcode == WebSocket::FRAME_OP_PING) {
		sendFrame(socket, payload, WebSocket::FRAME_OP_PONG | WebSocket::FRAME_FLAG_FIN);
	}
	else {
		message = GWMessage::fromJSON(payload);
		if (logger().debug()) {
			logger().debug(
				"received message " + message->toBriefString(),
				__FILE__, __LINE__);
		}
	}

	FastMutex::ScopedLock guard(m_lock);
	m_lastActivity.update();
	return message;
}
