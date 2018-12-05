#pragma once

#include <string>

#include <Poco/Logger.h>
#include <Poco/SharedPtr.h>

#include "core/Exporter.h"
#include "net/MqttClient.h"
#include "util/Loggable.h"

namespace BeeeOn {

class SensorDataFormatter;

class MqttExporter :
	public Exporter,
	protected Loggable {
public:
	MqttExporter();
	~MqttExporter();

	bool ship(const SensorData &data) override;

	void setTopic(const std::string &topic);

	void setQos(int qos);

	void setMqttClient(MqttClient::Ptr client);

	void setFormatter(const Poco::SharedPtr<SensorDataFormatter> formatter);

private:
	std::string m_topic;
	MqttMessage::QoS m_qos;
	std::string m_clientID;
	Poco::SharedPtr<SensorDataFormatter> m_formatter;
	MqttClient::Ptr m_mqtt;
};

}
