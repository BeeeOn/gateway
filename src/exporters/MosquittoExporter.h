#ifndef BEEEON_MQTT_EXPORTER_H
#define BEEEON_MQTT_EXPORTER_H

#include <string>
#include <mosquittopp.h>

#include <Poco/Logger.h>
#include <Poco/SharedPtr.h>

#include "core/Exporter.h"
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

	void setHost(const std::string &host);

	void setPort(int port);

	void setTopic(const std::string &topic);

	void setQos(int qos);

	/*
	 * Client name have to be unique for every exporter.
	 * In the case of running multiple exporters with the same client name,
	 * only one of them will work.
	 */
	void setClientName(const std::string &clientName);

	void setFormatter(const Poco::SharedPtr<SensorDataFormatter> formatter);

private:
	void connect();

	void disconnect();

	void throwMosquittoError(int returnCode);

	std::string m_host;
	int m_port;
	std::string m_topic;
	int m_qos;
	std::string m_clientName;
	Poco::SharedPtr<SensorDataFormatter> m_formatter;
	Poco::SharedPtr<mosqpp::mosquittopp> m_mq;
};

}

#endif // BEEEON_MQTT_EXPORTER_H
