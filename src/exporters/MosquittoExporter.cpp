#include <cstring>

#include <Poco/Exception.h>

#include "di/Injectable.h"
#include "exporters/MosquittoExporter.h"
#include "util/NullSensorDataFormatter.h"
#include "util/SensorDataFormatter.h"

BEEEON_OBJECT_BEGIN(BeeeOn, MosquittoExporter)
BEEEON_OBJECT_CASTABLE(Exporter)
BEEEON_OBJECT_PROPERTY("topic", &MosquittoExporter::setTopic)
BEEEON_OBJECT_PROPERTY("qos", &MosquittoExporter::setQos)
BEEEON_OBJECT_PROPERTY("formatter", &MosquittoExporter::setFormatter)
BEEEON_OBJECT_PROPERTY("mqttClient", &MosquittoExporter::setMqttClient)
BEEEON_OBJECT_END(BeeeOn, MosquittoExporter)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const static string DEFAULT_TOPIC = "BeeeOnOut";
const static string DEFAULT_CLIENT_ID = "GatewayExporterClient";

MosquittoExporter::MosquittoExporter():
	m_topic(DEFAULT_TOPIC),
	m_qos(MqttMessage::EXACTLY_ONCE),
	m_clientID(DEFAULT_CLIENT_ID)
{
}

MosquittoExporter::~MosquittoExporter()
{
}

void MosquittoExporter::setTopic(const string &topic)
{
	m_topic = topic;
}

void MosquittoExporter::setMqttClient(MosquittoClient::Ptr client)
{
	m_mqtt = client;
}

void MosquittoExporter::setFormatter(const SharedPtr<SensorDataFormatter> formatter)
{
	m_formatter = formatter;
}

bool MosquittoExporter::ship(const SensorData &data)
{
	MqttMessage msg = {
		m_topic,
		m_formatter->format(data),
		m_qos
	};

	try {
		m_mqtt->publish(msg);
	}
	catch (const Exception &ex) {
		logger().log(ex, __FILE__, __LINE__);
		return false;
	}

	return true;
}

void MosquittoExporter::setQos(const int qos)
{
	switch (qos) {
	case MqttMessage::MOST_ONCE:
	case MqttMessage::LEAST_ONCE:
	case MqttMessage::EXACTLY_ONCE:
		m_qos = static_cast<MqttMessage::QoS>(qos);
		break;
	default:
		throw InvalidArgumentException("QOS is out of range");
	}
}
