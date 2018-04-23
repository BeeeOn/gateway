#include "net/MqttMessage.h"

using namespace BeeeOn;
using namespace std;

MqttMessage::MqttMessage():
	m_qos(QoS::LEAST_ONCE)
{
}

MqttMessage::MqttMessage(
		const string &topic, const string &message, QoS qos):
	m_topic(topic),
	m_message(message),
	m_qos(qos)
{
}

string MqttMessage::topic() const
{
	return m_topic;
}

string MqttMessage::message() const
{
	return m_message;
}

MqttMessage::QoS MqttMessage::qos() const
{
	return m_qos;
}

bool MqttMessage::isEmpty() const
{
	return m_topic.empty() && m_message.empty();
}

bool MqttMessage::operator !() const
{
	return isEmpty();
}
