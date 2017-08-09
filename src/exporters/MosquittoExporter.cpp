#include <cstring>

#include <Poco/Exception.h>
#include <Poco/Net/NetException.h>

#include "di/Injectable.h"
#include "exporters/MosquittoExporter.h"
#include "util/NullSensorDataFormatter.h"
#include "util/SensorDataFormatter.h"

BEEEON_OBJECT_BEGIN(BeeeOn, MosquittoExporter)
BEEEON_OBJECT_CASTABLE(Exporter)
BEEEON_OBJECT_TEXT("host", &MosquittoExporter::setHost)
BEEEON_OBJECT_NUMBER("port", &MosquittoExporter::setPort)
BEEEON_OBJECT_TEXT("topic", &MosquittoExporter::setTopic)
BEEEON_OBJECT_NUMBER("qos", &MosquittoExporter::setQos)
BEEEON_OBJECT_TEXT("clientName", &MosquittoExporter::setClientName)
BEEEON_OBJECT_REF("formatter", &MosquittoExporter::setFormatter)
BEEEON_OBJECT_END(BeeeOn, MosquittoExporter)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const static string DEFAULT_HOST = "localhost";
const static int DEFAULT_PORT = 1883;
const static string DEFAULT_TOPIC = "BeeeOnOut";
const static int DEFAULT_QOS = 0;
const static string DEFAULT_CLIENT_NAME = "Gateway";
const static int KEEPALIVE_SEC = 60;
const static int QOS_LOW = 0;
const static int QOS_HIGH = 2;

MosquittoExporter::MosquittoExporter() :
	m_host(DEFAULT_HOST),
	m_port(DEFAULT_PORT),
	m_topic(DEFAULT_TOPIC),
	m_qos(DEFAULT_QOS),
	m_clientName(DEFAULT_CLIENT_NAME)
{
	mosqpp::lib_init();
}

MosquittoExporter::~MosquittoExporter()
{
	disconnect();
	mosqpp::lib_cleanup();
}

void MosquittoExporter::setHost(const string &host)
{
	m_host = host;
}

void MosquittoExporter::setPort(const int port)
{
	if (port < 0 || port > 65535)
		throw InvalidArgumentException("port is out of range");
	m_port = port;
}

void MosquittoExporter::setTopic(const string &topic)
{
	m_topic = topic;
}

void MosquittoExporter::setQos(const int qos)
{
	if (qos < QOS_LOW || qos > QOS_HIGH)
		throw InvalidArgumentException("QOS is out of range");
	m_qos = qos;
}

void MosquittoExporter::setClientName(const string &clientName)
{
	m_clientName = clientName;
}

void MosquittoExporter::setFormatter(const SharedPtr<SensorDataFormatter> formatter)
{
	m_formatter = formatter;
}

bool MosquittoExporter::ship(const SensorData &data)
{
	string msg = m_formatter->format(data);

	if (m_mq.isNull())
		connect();

	int res = m_mq->publish(NULL, m_topic.c_str(), msg.length(), msg.c_str(), m_qos);

	if (res == MOSQ_ERR_SUCCESS) {
		return true;
	}
	else {
		disconnect();
		throwMosquittoError(res);
	}
	return false;
}

void MosquittoExporter::connect()
{
	m_mq = new mosqpp::mosquittopp(m_clientName.c_str());

	if (logger().debug()) {
		logger().debug("Host: " + m_host + ":" + to_string(m_port)
			+ "  Topic: " + m_topic
			+ "  Qos: " + to_string(m_qos)
			+ "  ClientName: " + m_clientName,
			__FILE__, __LINE__);
	}

	int res = m_mq->connect_async(m_host.c_str(), m_port, KEEPALIVE_SEC);

	if (res == MOSQ_ERR_SUCCESS) {
		logger().information("connected to " + m_host + ":" + to_string(m_port));
	}
	else {
		m_mq = nullptr;
		throwMosquittoError(res);
	}

}

void MosquittoExporter::disconnect()
{
	if (m_mq.isNull())
		return;

	m_mq->disconnect();
	m_mq = nullptr;
}

void MosquittoExporter::throwMosquittoError(int returnCode)
{
	switch (returnCode) {
	case MOSQ_ERR_INVAL:
		throw InvalidArgumentException("the input parameters were invalid");
	case MOSQ_ERR_NOMEM:
		throw OutOfMemoryException("an out of memory condition occurred");
	case MOSQ_ERR_NO_CONN:
		throw IOException("the client is not connected to a broker");
	case MOSQ_ERR_PROTOCOL:
		throw ProtocolException("there is a protocol error communicating with the broker");
	case MOSQ_ERR_PAYLOAD_SIZE:
		throw ProtocolException("payloadlen is too large");
	case MOSQ_ERR_ERRNO:
		if (errno == ECONNREFUSED)
			throw Net::ConnectionRefusedException("failed to connect to mqtt broker");
		else
			throw SystemException("system call returned an error: " + string(strerror(errno)));
	default:
		throw IllegalStateException("unknown error");
	}
}
