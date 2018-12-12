#pragma once

#include <string>

#include <Poco/AutoPtr.h>
#include <Poco/Buffer.h>
#include <Poco/Clock.h>
#include <Poco/Net/SocketNotification.h>
#include <Poco/Net/WebSocket.h>

#include "core/GatewayInfo.h"
#include "loop/StoppableRunnable.h"
#include "loop/StopControl.h"
#include "server/AbstractGWSConnector.h"
#include "ssl/SSLClient.h"

namespace BeeeOn {

/**
 * @brief Implements the communication via WebSockets with the remote
 * server. Outgoing messages are prioritized based on the configured
 * GWSPriorityAssigner instance. Incoming messages are broadcasted
 * via the registered GWSListener instances. The GWSConnectorImpl takes
 * care just for the lowest-level communication details:
 *
 * - reconnecting in case of failures,
 * - sending messages,
 * - receiving messages,
 * - keep alive ping-pong.
 */
class GWSConnectorImpl :
	public AbstractGWSConnector,
	public StoppableRunnable {
public:
	GWSConnectorImpl();

	void setHost(const std::string &host);
	void setPort(int port);
	void setMaxMessageSize(int size);
	void setSSLConfig(SSLClient::Ptr config);
	void setReceiveTimeout(const Poco::Timespan &timeout);
	void setSendTimeout(const Poco::Timespan &timeout);
	void setReconnectDelay(const Poco::Timespan &delay);
	void setKeepAliveTimeout(const Poco::Timespan &timeout);
	void setMaxFailedReceives(int count);
	void setGatewayInfo(GatewayInfo::Ptr info);

	void run();
	void stop();

protected:
	void waitBeforeReconnect();

	Poco::SharedPtr<Poco::Net::WebSocket> connect(
		const std::string &host,
		int port) const;
	void performRegister(Poco::Net::WebSocket &socket) const;
	bool performOutput(Poco::Net::WebSocket &socket);
	void performPing(Poco::Net::WebSocket &socket);
	void checkPingTimeout() const;
	void outputLoop(
		StopControl::Run &run,
		Poco::Net::WebSocket &socket);

	/**
	 * @brief Wait while the output queues are empty. The waiting
	 * delay is driver by the keepAliveTimeout.
	 * @returns true if no keep-alive is necessary, false when
	 * the keepAliveTimeout has exceeded
	 */
	bool waitOutputs();

	/**
	 * @returns remaining timeout before keep-alive timeout exceeds
	 */
	const Poco::Timespan keepAliveRemaining() const;

	void onReadable(const Poco::AutoPtr<Poco::Net::ReadableNotification> &n);

	void sendMessage(
		Poco::Net::WebSocket &socket,
		const GWMessage &message) const;
	void sendFrame(
		Poco::Net::WebSocket &socket,
		const std::string &payload,
		const int flags) const;

	GWMessage::Ptr receiveMessage(Poco::Net::WebSocket &socket) const;
	int receiveFrame(
		Poco::Net::WebSocket &socket,
		Poco::Buffer<char> &buffer,
		int &flags) const;

private:
	std::string m_host;
	int m_port;
	size_t m_maxMessageSize;
	SSLClient::Ptr m_sslConfig;
	Poco::Timespan m_receiveTimeout;
	Poco::Timespan m_sendTimeout;
	Poco::Timespan m_reconnectDelay;
	Poco::Timespan m_keepAliveTimeout;
	int m_maxFailedReceives;
	GatewayInfo::Ptr m_gatewayInfo;

	mutable Poco::FastMutex m_sendLock;
	mutable Poco::FastMutex m_receiveLock;

	StopControl m_stopControl;
	mutable Poco::Clock m_lastActivity;
	mutable Poco::FastMutex m_lock;

	Poco::Clock m_lastPing;
	Poco::AtomicCounter m_receiveFailed;
};

}
