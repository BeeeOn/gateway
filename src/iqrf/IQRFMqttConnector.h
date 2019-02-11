#pragma once

#include <map>
#include <string>

#include <Poco/Clock.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>

#include "iqrf/IQRFJsonResponse.h"
#include "loop/StopControl.h"
#include "loop/StoppableRunnable.h"
#include "model/GlobalID.h"
#include "net/MqttClient.h"
#include "util/Loggable.h"
#include "util/WaitCondition.h"

namespace BeeeOn {

/**
 * @brief IQRFMqttConnector provides sending and receiving messages.
 * During data receiving, it is necessary to know identification of
 * JSON message (GlobalID). The identification of message is used
 * to bring sent and received message together.
 */
class IQRFMqttConnector final:
	public StoppableRunnable,
	private Loggable {
public:
	typedef Poco::SharedPtr<IQRFMqttConnector> Ptr;

	IQRFMqttConnector();

	void run() override;
	void stop() override;

	void setPublishTopic(const std::string &topic);

	void setMqttClient(
		MqttClient::Ptr mqttClient);

	void setDataTimeout(const Poco::Timespan &timeout);
	void setReceiveTimeout(const Poco::Timespan &timeout);
	void checkPublishTopic();

	/**
	 * @brief Send message using mqtt client to specific topic.
	 */
	void send(const std::string &msg);

	/**
	 * @brief Receive message with given timeout and identification
	 * of message.
	 */
	IQRFJsonResponse::Ptr receive(
		const GlobalID &id, const Poco::Timespan &timeout);

private:
	void removeExpiredMessages(const Poco::Timespan &timeout);

private:
	/**
	 * @brief Represents a received JSON message and time.
	 */
	struct ReceivedData {
		Poco::Clock receivedAt;
		IQRFJsonResponse::Ptr message;
	};

private:
	StopControl m_stopControl;
	std::map<GlobalID, ReceivedData> m_data;
	Poco::Timespan m_messageTimeout;
	Poco::Timespan m_receiveTimeout;

	WaitCondition m_waitCondition;
	Poco::FastMutex m_dataLock;

	MqttClient::Ptr m_mqttClient;
	std::string m_publishTopic;
};

}
