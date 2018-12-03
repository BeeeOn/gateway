#include <cstring>

#include <Poco/Exception.h>

#include "di/Injectable.h"
#include "exporters/MqttExporter.h"
#include "util/NullSensorDataFormatter.h"
#include "util/SensorDataFormatter.h"

BEEEON_OBJECT_BEGIN(BeeeOn, MqttExporter)
BEEEON_OBJECT_CASTABLE(Exporter)
BEEEON_OBJECT_PROPERTY("topic", &MqttExporter::setTopic)
BEEEON_OBJECT_PROPERTY("qos", &MqttExporter::setQos)
BEEEON_OBJECT_PROPERTY("formatter", &MqttExporter::setFormatter)
BEEEON_OBJECT_PROPERTY("mqttClient", &MqttExporter::setMqttClient)
BEEEON_OBJECT_END(BeeeOn, MqttExporter)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const static string DEFAULT_TOPIC = "BeeeOnOut";
const static string DEFAULT_CLIENT_ID = "GatewayExporterClient";

MqttExporter::MqttExporter():
	m_topic(DEFAULT_TOPIC),
	m_qos(MqttMessage::EXACTLY_ONCE),
	m_clientID(DEFAULT_CLIENT_ID)
{
}

MqttExporter::~MqttExporter()
{
}

void MqttExporter::setTopic(const string &topic)
{
	m_topic = topic;
}

void MqttExporter::setMqttClient(MqttClient::Ptr client)
{
	m_mqtt = client;
}

void MqttExporter::setFormatter(const SharedPtr<SensorDataFormatter> formatter)
{
	m_formatter = formatter;
}

bool MqttExporter::ship(const SensorData &data)
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

void MqttExporter::setQos(const int qos)
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
