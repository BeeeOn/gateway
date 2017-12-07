#pragma once

#include <Poco/AtomicCounter.h>
#include <Poco/AutoPtr.h>
#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/SharedPtr.h>
#include <Poco/Thread.h>
#include <Poco/Timespan.h>
#include <Poco/Timestamp.h>
#include <Poco/Util/Timer.h>

#include "commands/NewDeviceCommand.h"
#include "commands/ServerDeviceListCommand.h"
#include "commands/ServerLastValueCommand.h"
#include "core/CommandHandler.h"
#include "core/CommandSender.h"
#include "core/Exporter.h"
#include "core/GatewayInfo.h"
#include "gwmessage/GWDeviceAcceptRequest.h"
#include "gwmessage/GWListenRequest.h"
#include "gwmessage/GWMessage.h"
#include "gwmessage/GWMessage.h"
#include "gwmessage/GWPingRequest.h"
#include "gwmessage/GWRequest.h"
#include "gwmessage/GWResponse.h"
#include "gwmessage/GWSensorDataConfirm.h"
#include "gwmessage/GWSetValueRequest.h"
#include "gwmessage/GWUnpairRequest.h"
#include "loop/StoppableLoop.h"
#include "server/GWMessageContext.h"
#include "server/GWSOutputQueue.h"
#include "server/GWContextPoll.h"
#include "ssl/SSLClient.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief The GWServerConnector allows the BeeeOn Gateway to communicate
 * with the BeeeOn Server using WebSocket. It automatically connects
 * and registers the gateway after start or connection loss.
 *
 * There are 2 threads: sender and receiver. Sender's responsibility is reconnecting
 * to server and sending messages. Receiver's responsibility is to to receive messages
 * from server and handling them.
 */
class GWServerConnector :
	public StoppableLoop,
	public CommandSender,
	public CommandHandler,
	public Exporter,
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
	void setBusySleep(const Poco::Timespan &timeout);
	void setResendTimeout(const Poco::Timespan &timeout);
	void setMaxMessageSize(int size);
	void setGatewayInfo(Poco::SharedPtr<GatewayInfo> info);
	void setSSLConfig(Poco::SharedPtr<SSLClient> config);
	void setInactiveMultiplier(int multiplier);

	bool accept(const Command::Ptr cmd) override;
	void handle(Command::Ptr cmd, Answer::Ptr answer) override;

	bool ship(const SensorData &data) override;

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
	 * Starts sender in separate thread.
	 */
	void startSender();

	/**
	 * Connects and registers the gateway to the server, if signalled by the
	 * event m_requestReconnect. Sends messages after connection is established.
	 */
	void runSender();

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
	 * Perform reconnect and register gateway to server,
	 * after this call connection is considered established.
	 */
	void reconnect();

	/**
	 * Signals the sender to reconnect.
	 */
	void markDisconnected();

	/**
	 * Send ping frame to server.
	 */
	void sendPing();

	/**
	 * Forward given context to server. If nullptr is given, waits
	 * for readyToSendEvent() to be notified or send ping frame after timeout.
	 */
	void forwardContext(GWMessageContext::Ptr context);

	/**
	 * Dequeue context from outputQueue and forward is to server.
	 */
	void forwardOutputQueue();
	void sendMessage(const GWMessage::Ptr message);
	void sendMessageUnlocked(const GWMessage::Ptr message);

	void doNewDeviceCommand(NewDeviceCommand::Ptr cmd, Answer::Ptr answer);
	void doDeviceListCommand(ServerDeviceListCommand::Ptr cmd, Answer::Ptr answer);
	void doLastValueCommand(ServerLastValueCommand::Ptr cmd, Answer::Ptr answer);

	void dispatchServerCommand(Command::Ptr cmd, const GlobalID &id, GWResponse::Ptr response);

	/**
	 * Handle request received from server. one of the specific "handle*Reqest()" method
	 * is called to process request. An appropriate Command is dispatched eventually.
	 */
	void handleRequest(GWRequest::Ptr request);

	void handleDeviceAcceptRequest(GWDeviceAcceptRequest::Ptr request);
	void handleListenRequest(GWListenRequest::Ptr request);
	void handleSetValueRequest(GWSetValueRequest::Ptr request);
	void handleUnpairRequest(GWUnpairRequest::Ptr request);

	/**
	 * Handle received response message, corresponding request is first
	 * found in GWContextPoll and its result is set.
	 */
	void handleResponse(GWResponse::Ptr response);

	/**
	 * Handle Ack message, corresponding context is found
	 * in GWContextPoll and remove it from m_contextPoll.
	 */
	void handleAck(GWAck::Ptr ack);

	/**
	 * Handle confirmation of receiving exported SensorData, this will remove
	 * message from GWContextPoll so its not resend.
	 */
	void handleSensorDataConfirm(GWSensorDataConfirm::Ptr confirm);

	/**
	 * Handle generic message from server, specific handle*() method is
	 * invoked based on message type.
	 */
	void handleMessage(GWMessage::Ptr msg);

	void enqueueFinishedAnswers();

	/**
	 * Returns true if too much time elapsed since we last
	 * received message from server. In this case, connection
	 * is considere broken and must be reconnected.
	 */
	bool connectionSeemsBroken();
	void updateLastReceived();

	GWMessage::Ptr receiveMessageUnlocked();

	Poco::Event &readyToSendEvent();

private:
	std::string m_host;
	int m_port;
	Poco::Timespan m_pollTimeout;
	Poco::Timespan m_receiveTimeout;
	Poco::Timespan m_sendTimeout;
	Poco::Timespan m_retryConnectTimeout;
	Poco::Timespan m_busySleep;
	Poco::Timespan m_resendTimeout;
	size_t m_maxMessageSize;
	Poco::SharedPtr<GatewayInfo> m_gatewayInfo;
	Poco::SharedPtr<SSLClient> m_sslConfig;
	Poco::Timestamp m_lastReceived;
	Poco::FastMutex m_lastReceivedMutex;
	int m_inactiveMultiplier;

	Poco::Buffer<char> m_receiveBuffer;
	Poco::SharedPtr<Poco::Net::WebSocket> m_socket;
	Poco::FastMutex m_receiveMutex;
	Poco::FastMutex m_sendMutex;
	Poco::Thread m_senderThread;
	Poco::Thread m_receiverThread;

	Poco::AtomicCounter m_isConnected;
	Poco::Event m_connectedEvent;

	Poco::AtomicCounter m_stop;
	Poco::Event m_stopEvent;

	GWContextPoll m_contextPoll;
	GWSOutputQueue m_outputQueue;
	Poco::Util::Timer m_timer;
};

}
