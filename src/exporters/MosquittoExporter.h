#pragma once

#include <string>
#include <mosquittopp.h>

#include <Poco/Logger.h>
#include <Poco/SharedPtr.h>

#include "core/Exporter.h"
#include "net/MosquittoClient.h"
#include "util/Loggable.h"

namespace BeeeOn {

class SensorDataFormatter;

class MosquittoExporter :
	public Exporter,
	protected Loggable {
public:
	MosquittoExporter();
	~MosquittoExporter();

	bool ship(const SensorData &data) override;

	void setTopic(const std::string &topic);

	void setQos(int qos);

	void setMqttClient(MosquittoClient::Ptr client);

	void setFormatter(const Poco::SharedPtr<SensorDataFormatter> formatter);

private:
	std::string m_topic;
	MqttMessage::QoS m_qos;
	std::string m_clientID;
	Poco::SharedPtr<SensorDataFormatter> m_formatter;
	MosquittoClient::Ptr m_mqtt;
};

}
