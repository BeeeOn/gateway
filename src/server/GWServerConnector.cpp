#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/NetException.h>

#include "di/Injectable.h"
#include "gwmessage/GWGatewayAccepted.h"
#include "gwmessage/GWGatewayRegister.h"
#include "server/GWServerConnector.h"

BEEEON_OBJECT_BEGIN(BeeeOn, GWServerConnector)
BEEEON_OBJECT_CASTABLE(StoppableLoop)
BEEEON_OBJECT_TEXT("host", &GWServerConnector::setHost)
BEEEON_OBJECT_NUMBER("port", &GWServerConnector::setPort)
BEEEON_OBJECT_TIME("pollTimeout", &GWServerConnector::setPollTimeout)
BEEEON_OBJECT_TIME("receiveTimeout", &GWServerConnector::setReceiveTimeout)
BEEEON_OBJECT_TIME("sendTimeout", &GWServerConnector::setSendTimeout)
BEEEON_OBJECT_TIME("retryConnectTimeout", &GWServerConnector::setRetryConnectTimeout)
BEEEON_OBJECT_NUMBER("maxMessageSize", &GWServerConnector::setMaxMessageSize)
BEEEON_OBJECT_REF("sslConfig", &GWServerConnector::setSSLConfig)
BEEEON_OBJECT_REF("gatewayInfo", &GWServerConnector::setGatewayInfo)
BEEEON_OBJECT_END(BeeeOn, GWServerConnector)

using namespace std;
using namespace Poco;
using namespace Poco::Net;
using namespace BeeeOn;

static const Timespan RECONNECT_TIMEOUT = Timespan::SECONDS * 10;

GWServerConnector::GWServerConnector():
	m_port(0),
	m_pollTimeout(250 * Timespan::MILLISECONDS),
	m_receiveTimeout(3 * Timespan::SECONDS),
	m_sendTimeout(1 * Timespan::SECONDS),
	m_retryConnectTimeout(1 * Timespan::SECONDS),
	m_maxMessageSize(4096),
	m_receiveBuffer(m_maxMessageSize),
	m_isConnected(false),
	m_stop(false)
{
}

void GWServerConnector::start()
{
	m_receiveBuffer.resize(m_maxMessageSize, false);

	m_stop = false;

	startSender();
	startReceiver();
}

void GWServerConnector::stop()
{
	m_stop = true;
	m_stopEvent.set();
	m_connectedEvent.set();

	m_senderThread.join();
	m_receiverThread.join();

	disconnectUnlocked();
}

void GWServerConnector::startSender()
{
	m_senderThread.startFunc([this](){
		runSender();
	});
}

void GWServerConnector::runSender()
{
	while (!m_stop) {

		if (!m_isConnected) {
			reconnect();
			continue;
		}

		m_stopEvent.tryWait(RECONNECT_TIMEOUT.totalMilliseconds());
	}
}

void GWServerConnector::reconnect()
{
	FastMutex::ScopedLock sendGuard(m_sendMutex);
	FastMutex::ScopedLock receiveGuard(m_receiveMutex);
	disconnectUnlocked();
	connectAndRegisterUnlocked();
	m_isConnected = true;
	m_connectedEvent.set();
}

void GWServerConnector::startReceiver()
{
	m_receiverThread.startFunc([this](){
		runReceiver();
	});
}

void GWServerConnector::runReceiver()
{
	while (!m_stop) {
		if (!m_isConnected) {
			m_connectedEvent.wait();
			continue;
		}

		FastMutex::ScopedLock guard(m_receiveMutex);
		try {
			if (m_socket.isNull()) {
				throw ConnectionResetException(
					"server connection is not initialized");
			}

			if (!m_socket->poll(m_pollTimeout, Socket::SELECT_READ))
				continue;

			GWMessage::Ptr msg = receiveMessageUnlocked();
			poco_information(logger(), "received new message of type: "
					+ msg->type().toString());
		}
		catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
			markDisconnected();
		}
		catch (const exception &e) {
			logger().critical(e.what(), __FILE__, __LINE__);
			markDisconnected();
		}
		catch (...) {
			logger().critical("unknown error", __FILE__, __LINE__);
			markDisconnected();
		}
	}
}

void GWServerConnector::markDisconnected()
{
	m_isConnected = false;
}

bool GWServerConnector::connectUnlocked()
{
	logger().information("connecting to server "
		+ m_host + ":" + to_string(m_port), __FILE__, __LINE__);

	try {
		HTTPRequest request(HTTPRequest::HTTP_1_1);
		HTTPResponse response;

		if (m_sslConfig.isNull()) {
			HTTPClientSession cs(m_host, m_port);
			m_socket = new WebSocket(cs, request, response);
		}
		else {
			HTTPSClientSession cs(m_host, m_port, m_sslConfig->context());
			m_socket = new WebSocket(cs, request, response);
		}

		m_socket->setReceiveTimeout(m_receiveTimeout);
		m_socket->setSendTimeout(m_sendTimeout);

		logger().information("successfully connected to server",
			__FILE__, __LINE__);
		return true;
	}
	catch (const Exception &e) {
		logger().log(e, __FILE__, __LINE__);
		return false;
	}
}

bool GWServerConnector::registerUnlocked()
{
	logger().information("registering gateway "
			+ m_gatewayInfo->gatewayID().toString(), __FILE__, __LINE__);

	try {
		GWGatewayRegister::Ptr registerMsg = new GWGatewayRegister;
		registerMsg->setGatewayID(m_gatewayInfo->gatewayID());
		registerMsg->setIPAddress(m_socket->address().host());
		registerMsg->setVersion(m_gatewayInfo->version());

		sendMessageUnlocked(registerMsg);

		GWMessage::Ptr msg = receiveMessageUnlocked();

		if (msg.cast<GWGatewayAccepted>().isNull()) {
			logger().error("unexpected response " + msg->type().toString(),
				__FILE__, __LINE__);
			return false;
		}

		logger().information("successfully registered", __FILE__, __LINE__);
		return true;
	}
	catch (const Exception &e) {
		logger().log(e, __FILE__, __LINE__);
		return false;
	}
}

void GWServerConnector::connectAndRegisterUnlocked()
{
	while (!m_stop) {
		try {
			if (connectUnlocked() && registerUnlocked())
				break;
		}
		catch (const exception &e) {
			logger().critical(e.what(), __FILE__, __LINE__);
		}
		catch (...) {
			logger().critical("unknown error", __FILE__, __LINE__);
		}

		if (m_stopEvent.tryWait(m_retryConnectTimeout.totalMilliseconds()))
			break;
	}
}

void GWServerConnector::disconnectUnlocked()
{
	if (!m_socket.isNull()) {
		m_socket = nullptr;

		logger().information("disconnected", __FILE__, __LINE__);
	}
}

void GWServerConnector::sendMessage(const GWMessage::Ptr message)
{
	FastMutex::ScopedLock guard(m_sendMutex);

	if (m_socket.isNull())
		throw ConnectionResetException("server connection is not initialized");

	sendMessageUnlocked(message);
}

void GWServerConnector::sendMessageUnlocked(const GWMessage::Ptr message)
{
	const string &msg = message->toString();

	if (logger().trace())
		logger().trace("send:\n" + msg, __FILE__, __LINE__);

	m_socket->sendFrame(msg.c_str(), msg.length());
}

GWMessage::Ptr GWServerConnector::receiveMessageUnlocked()
{
	int flags;
	int ret = m_socket->receiveFrame(m_receiveBuffer.begin(),
			m_receiveBuffer.size(), flags);

	if (ret <= 0 || (flags & WebSocket::FRAME_OP_CLOSE))
		throw ConnectionResetException("server connection closed");

	string data(m_receiveBuffer.begin(), ret);

	if (logger().trace())
		logger().trace("received:\n" + data, __FILE__, __LINE__);

	return GWMessage::fromJSON(data);
}

void GWServerConnector::setHost(const string &host)
{
	m_host = host;
}

void GWServerConnector::setPort(int port)
{
	if (port < 0 || port > 65535)
		throw InvalidArgumentException("port must be in range 0..65535");

	m_port = port;
}

void GWServerConnector::setPollTimeout(const Timespan &timeout)
{
	if (timeout < 0)
		throw InvalidArgumentException("receive timeout must be non negative");

	m_pollTimeout = timeout;
}

void GWServerConnector::setReceiveTimeout(const Timespan &timeout)
{
	if (timeout < 0)
		throw InvalidArgumentException("receive timeout must be non negative");

	m_receiveTimeout = timeout;
}

void GWServerConnector::setSendTimeout(const Timespan &timeout)
{
	if (timeout < 0)
		throw InvalidArgumentException("send timeout must be non negative");

	m_sendTimeout = timeout;
}

void GWServerConnector::setRetryConnectTimeout(const Timespan &timeout)
{
	if (timeout < 0)
		throw InvalidArgumentException("retryConnectTimeout must be non negative");

	m_retryConnectTimeout = timeout;
}

void GWServerConnector::setMaxMessageSize(int size)
{
	if (size <= 0)
		throw InvalidArgumentException("size must be non negative");

	m_maxMessageSize = size;
}

void GWServerConnector::setGatewayInfo(SharedPtr<GatewayInfo> info)
{
	m_gatewayInfo = info;
}

void GWServerConnector::setSSLConfig(SharedPtr<SSLClient> config)
{
	m_sslConfig = config;
}
