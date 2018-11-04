#pragma once

#include <string>

namespace BeeeOn {

class MqttMessage {
public:
	enum QoS {
		MOST_ONCE = 0,
		LEAST_ONCE = 1,
		EXACTLY_ONCE = 2,
	};

	MqttMessage();
	MqttMessage(
		const std::string &topic,
		const std::string &message,
		QoS oqs = QoS::LEAST_ONCE);

	std::string topic() const;
	std::string message() const;
	QoS qos() const;

	bool isEmpty() const;

	bool operator !() const;

private:
	std::string m_topic;
	std::string m_message;
	MqttMessage::QoS m_qos;
};

}
