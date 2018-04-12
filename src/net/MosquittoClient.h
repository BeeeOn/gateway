#ifndef BEEEON_MOSQUITTO_CLIENT_H
#define BEEEON_MOSQUITTO_CLIENT_H

#include <list>
#include <queue>
#include <set>
#include <string>

#include <Poco/AtomicCounter.h>
#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>

#include <mosquittopp.h>

#include "loop/StoppableRunnable.h"
#include "net/MqttMessage.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * Mosquitto client runs in a thread. Before its run
 * it is necessary to set following parameters:
 *
 *  - host: default localhost
 *  - port: default 1883
 *  - reconnect wait timeout: 5 s
 *  - client id: default - empty
 */
class MosquittoClient:
	Loggable,
	protected mosqpp::mosquittopp,
	public StoppableRunnable {
public:
	typedef Poco::SharedPtr<MosquittoClient> Ptr;

	MosquittoClient();
	~MosquittoClient();

	/**
	 * Keeps communication between the client and broker working.
	 *
	 * @see: https://mosquitto.org/api/files/mosquitto-h.html#mosquitto_loop
	 */
	void run() override;
	void stop() override;

	void setHost(const std::string &host);
	void setPort(int port);

	/**
	 * Subscribe to a topic.
	 */
	void setSubTopics(const std::list<std::string> &subTopics);

	/**
	 * Timeout between reconnecting to the server when server
	 * connection is lost.
	 */
	void setReconnectTimeout(const Poco::Timespan &timeout);

	void setClientID(const std::string &id);
	std::string clientID() const;

	/**
	 * Publish a message on a given topic.
	 */
	void publish(const MqttMessage &msq);

	/**
	 * Waiting for a new message according to given timeout.
	 * Returns message, if some message is in queue, otherwise
	 * waiting for a new message or timeout exception.
	 *
	 * This method should not be called by multiple threads,
	 * received message could be given to only one thread.
	 *
	 * Timeout:
	 *  - 0 - non-blocking
	 *  - negative - blocking
	 *  - positive blocking with timeout
	 */
	MqttMessage receive(const Poco::Timespan &timeout);

protected:
	virtual std::string buildClientID() const;

	/**
	 * Non-blocking connection to broker request.
	 * After set client name must be call MosquittoClient::connect();
	 */
	void connect();

private:
	/**
	 * Setting clientID and connection to the server. After successful connection,
	 * it is necessary to set m_isConnected variable to true.
	 */
	bool initConnection();

	void subscribeToAll();

	void throwMosquittoError(int returnCode) const;

	/**
	 * Message variable contains data and name of topic. This variable and associated
	 * memory will be freed by the library after the callback completes.
	 *
	 * @see: https://mosquitto.org/api/files/mosquitto-h.html#mosquitto_message_callback_set
	 */
	void on_message(const struct mosquitto_message *message) override;

	/**
	 * Returns message from queue.
	 */
	MqttMessage nextMessage();

private:
	std::string m_clientID;
	std::string m_host;
	Poco::Timespan m_reconnectTimeout;
	int m_port;
	std::set<std::string> m_subTopics;
	Poco::AtomicCounter m_stop;
	Poco::Event m_receiveEvent;
	Poco::Event m_reconnectEvent;
	Poco::FastMutex m_queueMutex;
	std::queue<MqttMessage> m_msgQueue;
};

}

#endif
