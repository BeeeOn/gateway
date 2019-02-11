#include <Poco/Clock.h>
#include <Poco/Error.h>
#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/Thread.h>
#include <Poco/Net/NetException.h>

#include "di/Injectable.h"
#include "net/MosquittoClient.h"

BEEEON_OBJECT_BEGIN(BeeeOn, MosquittoClient)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(MqttClient)
BEEEON_OBJECT_PROPERTY("port", &MosquittoClient::setPort)
BEEEON_OBJECT_PROPERTY("host", &MosquittoClient::setHost)
BEEEON_OBJECT_PROPERTY("clientID", &MosquittoClient::setClientID)
BEEEON_OBJECT_PROPERTY("reconnectTimeout", &MosquittoClient::setReconnectTimeout)
BEEEON_OBJECT_PROPERTY("subTopics", &MosquittoClient::setSubTopics)
BEEEON_OBJECT_END(BeeeOn, MosquittoClient)

using namespace BeeeOn;
using namespace Poco;
using namespace std;
using namespace mosqpp;

static const string DEFAULT_HOST = "localhost";
static const int DEFAULT_PORT = 1883;
static const Timespan RECONNECT_WAIT_TIMEOUT = 5 * Timespan::SECONDS;
static const int MAXIMUM_MESSAGE_SIZE = 1024;

MosquittoClient::MosquittoClient():
	m_host(DEFAULT_HOST),
	m_reconnectTimeout(RECONNECT_WAIT_TIMEOUT),
	m_port(DEFAULT_PORT),
	m_stop(false)
{
	// Mandatory initialization for mosquitto library
	mosqpp::lib_init();
}

MosquittoClient::~MosquittoClient()
{
	// never throws
	disconnect();
	mosqpp::lib_cleanup();
}

bool MosquittoClient::initConnection()
{
	const string clientID = buildClientID();
	reinitialise(clientID.c_str(), true);

	try {
		connect();
		subscribeToAll();
		return true;
	}
	catch (const Exception &ex) {
		logger().log(ex, __FILE__, __LINE__);
		return false;
	}
}

string MosquittoClient::buildClientID() const
{
	if (m_clientID.empty())
		throw IllegalStateException("client ID is not set");

	return m_clientID;
}

void MosquittoClient::publish(const MqttMessage &msg)
{
	int res = mosquittopp::publish(
		NULL,
		msg.topic().c_str(),
		msg.message().length(),
		msg.message().c_str(),
		msg.qos());

	if (res != MOSQ_ERR_SUCCESS)
		throwMosquittoError(res);
}

void MosquittoClient::connect()
{
	// non blocking connection to broker request
	int ret = connect_async(m_host.c_str(), m_port);

	if (ret != MOSQ_ERR_SUCCESS)
		throwMosquittoError(ret);
}

MqttMessage MosquittoClient::nextMessage()
{
	FastMutex::ScopedLock guard(m_queueMutex);
	MqttMessage msg;

	if (m_msgQueue.empty())
		return msg;

	msg = m_msgQueue.front();
	m_msgQueue.pop();

	return msg;
}

MqttMessage MosquittoClient::receive(const Timespan &timeout)
{
	const Poco::Clock now;

	while (!m_stop) {
		if (now.isElapsed(timeout.totalMicroseconds()) && timeout > 0)
			throw TimeoutException("receive timeout expired");

		MqttMessage msg = nextMessage();
		if (!msg.isEmpty())
			return msg;

		if (timeout < 0) {
			m_receiveEvent.wait();
		}
		else {
			Timespan waitTime = timeout.totalMicroseconds() - now.elapsed();

			if (waitTime <= 0)
				throw TimeoutException("receive timeout expired");

			if (waitTime.totalMilliseconds() < 1)
				waitTime = 1 * Timespan::MILLISECONDS;

			m_receiveEvent.wait(waitTime.totalMilliseconds());
		}
	}

	return {};
}

void MosquittoClient::on_message(const struct mosquitto_message *message)
{
	if (message->payloadlen > MAXIMUM_MESSAGE_SIZE) {
		throw RangeException(
			"maximum message size ("
			+ to_string(MAXIMUM_MESSAGE_SIZE)
			+ ") was exceeded");
	}

	FastMutex::ScopedLock guard(m_queueMutex);
	m_msgQueue.push({
		message->topic,
		string(reinterpret_cast<const char *>(message->payload), message->payloadlen)
	});

	m_receiveEvent.set();
}

void MosquittoClient::run()
{
	while (!m_stop) {
		if (initConnection())
			break;

		m_reconnectEvent.tryWait(m_reconnectTimeout.totalMilliseconds());
	}

	while (!m_stop) {
		int rc = loop();
		if (rc != MOSQ_ERR_SUCCESS) {
			m_reconnectEvent.tryWait(m_reconnectTimeout.totalMilliseconds());

			logger().trace("trying to reconnect", __FILE__, __LINE__);
			reconnect();
		}
	}
}

void MosquittoClient::stop()
{
	m_stop = true;
	m_receiveEvent.set();
	m_reconnectEvent.set();
}

void MosquittoClient::setSubTopics(const list<string> &subTopics)
{
	for (const auto &topic : subTopics) {
		const auto it = m_subTopics.emplace(topic);
		if (!it.second) {
			logger().warning(
				"duplicated subscription topic "
				+ topic,
				__FILE__, __LINE__);
		}
	}
}

void MosquittoClient::subscribeToAll()
{
	for (const auto &topic : m_subTopics) {
		int ret = mosquittopp::subscribe(NULL, topic.c_str());
		if (ret != MOSQ_ERR_SUCCESS)
			throwMosquittoError(ret);
	}
}

void MosquittoClient::setPort(int port)
{
	if (port < 0 || port > 65535)
		throw InvalidArgumentException("port is out of range");

	m_port = port;
}

void MosquittoClient::setHost(const string &host)
{
	m_host = host;
}

void MosquittoClient::setReconnectTimeout(const Timespan &timeout)
{
	if (timeout.totalSeconds() <= 0) {
		throw InvalidArgumentException(
			"reconnect timeout time must be at least a second");
	}

	m_reconnectTimeout = timeout;
}

void MosquittoClient::setClientID(const string &id)
{
	m_clientID = id;
}

string MosquittoClient::clientID() const
{
	return m_clientID;
}

void MosquittoClient::throwMosquittoError(int returnCode) const
{
	switch (returnCode) {
	case MOSQ_ERR_INVAL:
		throw InvalidArgumentException(Error::getMessage(returnCode));
	case MOSQ_ERR_NOMEM:
		throw OutOfMemoryException(Error::getMessage(returnCode));
	case MOSQ_ERR_NO_CONN:
		throw IOException(Error::getMessage(returnCode));
	case MOSQ_ERR_PROTOCOL:
		throw ProtocolException(Error::getMessage(returnCode));
	case MOSQ_ERR_PAYLOAD_SIZE:
		throw ProtocolException(Error::getMessage(returnCode));
	case MOSQ_ERR_ERRNO:
		if (errno == ECONNREFUSED)
			throw Net::ConnectionRefusedException(Error::getMessage(returnCode));
		else
			throw SystemException("system call returned an error: " + Error::getMessage(returnCode));
	default:
		throw IllegalStateException(Error::getMessage(returnCode));
	}
}
