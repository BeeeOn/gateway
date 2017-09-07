#ifndef GATEWAY_SERVER_CONNECTOR_H
#define GATEWAY_SERVER_CONNECTOR_H

#include <Poco/AtomicCounter.h>
#include <Poco/AutoPtr.h>
#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/Thread.h>
#include <Poco/Net/WebSocket.h>

#include "core/GatewayInfo.h"
#include "gwmessage/GWMessage.h"
#include "loop/StoppableLoop.h"
#include "ssl/SSLClient.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief The GWServerConnector allows the BeeeOn Gateway to communicate
 * with the BeeeOn Server using WebSocket. It automatically connects
 * and registers the gateway after start or connection loss.
 *
 * There are 3 threads, connector, receiver and sender (sender is currently
 * not implemented). Connector takes care about connecting to the server and
 * reconnecting if signalled. Receiver using poll takes care about receiving
 * messages from the server and detecting connection loss. Sender send queued
 * messages and export measured sensor data.
 */
class GWServerConnector :
	public StoppableLoop,
	protected Loggable {
public:
	typedef Poco::SharedPtr<GWServerConnector> Ptr;

	GWServerConnector();

	void start() override;
	void stop() override;

	void setHost(const std::string &host);
	void setPort(int port);
	void setPollTimeout(const Poco::Timespan &timeout);
	void setReceiveTimeout(const Poco::Timespan &timeout);
	void setSendTimeout(const Poco::Timespan &timeout);
	void setRetryConnectTimeout(const Poco::Timespan &timeout);
	void setMaxMessageSize(int size);
	void setGatewayInfo(Poco::SharedPtr<GatewayInfo> info);
	void setSSLConfig(Poco::SharedPtr<SSLClient> config);

private:
	/**
	 * Starts receiver in separate thread. The runReceiver() method is invoked
	 * after event m_connected is signalled.
	 */
	void startReceiver();

	/**
	 * Poll socket and receive messages in loop. In case of the connection loss,
	 * invalid message or some other problem requests reconnect,
	 */
	void runReceiver();

	/**
	 * Starts connector in separate thread.
	 */
	void startConnector();

	/**
	 * Connects and registers the gateway to the server, if signalled by the
	 * event m_requestReconnect.
	 */
	void runConnector();

	/**
	 * Tries connect to the server with WebSocket and returns status
	 * of the operation.
	 */
	bool connectUnlocked();

	/**
	 * Tries register to the server and returns status of the operation.
	 */
	bool registerUnlocked();

	/**
	 * Connects and registers to the server till success.
	 */
	void connectAndRegisterUnlocked();

	/**
	 * Disconnects from the server.
	 */
	void disconnectUnlocked();

	/**
	 * Signals the connector to reconnect and then wait till connected.
	 */
	void requestReconnect();

	void sendMessage(const GWMessage::Ptr message);
	void sendMessageUnlocked(const GWMessage::Ptr message);

	GWMessage::Ptr receiveMessageUnlocked();

private:
	std::string m_host;
	int m_port;
	Poco::Timespan m_pollTimeout;
	Poco::Timespan m_receiveTimeout;
	Poco::Timespan m_sendTimeout;
	Poco::Timespan m_retryConnectTimeout;
	size_t m_maxMessageSize;
	Poco::SharedPtr<GatewayInfo> m_gatewayInfo;
	Poco::SharedPtr<SSLClient> m_sslConfig;

	Poco::Buffer<char> m_receiveBuffer;
	Poco::SharedPtr<Poco::Net::WebSocket> m_socket;
	Poco::FastMutex m_receiveMutex;
	Poco::FastMutex m_sendMutex;
	Poco::Thread m_connectorThread;
	Poco::Thread m_receiverThread;
	Poco::FastMutex m_reconnectMutex;
	Poco::Event m_requestReconnect;
	Poco::Event m_connected;
	Poco::AtomicCounter m_stop;
	Poco::Event m_stopEvent;
};

}

#endif
