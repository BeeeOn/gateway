#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/NetException.h>

#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "commands/ServerDeviceListResult.h"
#include "commands/ServerLastValueResult.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "gwmessage/GWDeviceListRequest.h"
#include "gwmessage/GWDeviceListResponse.h"
#include "gwmessage/GWGatewayAccepted.h"
#include "gwmessage/GWGatewayRegister.h"
#include "gwmessage/GWLastValueRequest.h"
#include "gwmessage/GWLastValueResponse.h"
#include "gwmessage/GWNewDeviceRequest.h"
#include "gwmessage/GWResponseWithAck.h"
#include "gwmessage/GWSensorDataConfirm.h"
#include "server/GWServerConnector.h"
#include "server/ServerAnswer.h"

BEEEON_OBJECT_BEGIN(BeeeOn, GWServerConnector)
BEEEON_OBJECT_CASTABLE(StoppableLoop)
BEEEON_OBJECT_CASTABLE(Exporter)
BEEEON_OBJECT_CASTABLE(CommandHandler)
BEEEON_OBJECT_TEXT("host", &GWServerConnector::setHost)
BEEEON_OBJECT_NUMBER("port", &GWServerConnector::setPort)
BEEEON_OBJECT_TIME("pollTimeout", &GWServerConnector::setPollTimeout)
BEEEON_OBJECT_TIME("receiveTimeout", &GWServerConnector::setReceiveTimeout)
BEEEON_OBJECT_TIME("sendTimeout", &GWServerConnector::setSendTimeout)
BEEEON_OBJECT_TIME("retryConnectTimeout", &GWServerConnector::setRetryConnectTimeout)
BEEEON_OBJECT_TIME("busySleep", &GWServerConnector::setBusySleep)
BEEEON_OBJECT_TIME("resendTimeoutTimeout", &GWServerConnector::setResendTimeout)
BEEEON_OBJECT_NUMBER("maxMessageSize", &GWServerConnector::setMaxMessageSize)
BEEEON_OBJECT_NUMBER("inactiveMultiplier", &GWServerConnector::setInactiveMultiplier)
BEEEON_OBJECT_REF("sslConfig", &GWServerConnector::setSSLConfig)
BEEEON_OBJECT_REF("gatewayInfo", &GWServerConnector::setGatewayInfo)
BEEEON_OBJECT_REF("commandDispatcher", &GWServerConnector::setCommandDispatcher)
BEEEON_OBJECT_END(BeeeOn, GWServerConnector)

using namespace std;
using namespace Poco;
using namespace Poco::Net;
using namespace BeeeOn;

GWServerConnector::GWServerConnector():
	m_port(0),
	m_pollTimeout(250 * Timespan::MILLISECONDS),
	m_receiveTimeout(3 * Timespan::SECONDS),
	m_sendTimeout(1 * Timespan::SECONDS),
	m_retryConnectTimeout(1 * Timespan::SECONDS),
	m_busySleep(30 * Timespan::SECONDS),
	m_resendTimeout(20 * Timespan::SECONDS),
	m_maxMessageSize(4096),
	m_inactiveMultiplier(5),
	m_receiveBuffer(m_maxMessageSize),
	m_isConnected(false),
	m_stop(false),
	m_outputQueue(readyToSendEvent())
{
}

void GWServerConnector::start()
{
	m_receiveBuffer.resize(m_maxMessageSize, false);

	m_stop = false;

	startSender();
	startReceiver();
}

void GWServerConnector::stop()
{
	m_stop = true;
	m_stopEvent.set();
	readyToSendEvent().set();
	m_connectedEvent.set();

	m_senderThread.join();
	m_receiverThread.join();

	disconnectUnlocked();

	m_timer.cancel(true);
	m_outputQueue.clear();
	m_contextPoll.clear();
}

void GWServerConnector::startSender()
{
	m_senderThread.startFunc([this](){
		runSender();
	});
}

void GWServerConnector::runSender()
{
	while (!m_stop) {

		if (!m_isConnected) {
			reconnect();
			continue;
		}

		forwardOutputQueue();
	}
}

void GWServerConnector::forwardOutputQueue()
{
	try {
		enqueueFinishedAnswers();
		GWMessageContext::Ptr context = m_outputQueue.dequeue();
		if (!context.isNull()) {
			forwardContext(context);
		}
		else {
			if (!readyToSendEvent().tryWait(m_busySleep.totalMilliseconds()))
				sendPing();
		}
			if (connectionSeemsBroken())
				markDisconnected();
	}
	catch (const Exception &e) {
		logger().log(e, __FILE__, __LINE__);
		markDisconnected();
	}
	catch (const exception &e) {
		logger().critical(e.what(), __FILE__, __LINE__);
		markDisconnected();
	}
	catch (...) {
		logger().critical("unknown error", __FILE__, __LINE__);
		markDisconnected();
	}
}

void GWServerConnector::forwardContext(GWMessageContext::Ptr context)
{
	if (!context.cast<GWTimedContext>().isNull()) {
		GWTimedContext::Ptr timedContext = context.cast<GWTimedContext>();

		const GlobalID &id = timedContext->id();

		LambdaTimerTask::Ptr task = new LambdaTimerTask(
			[this, id](){
				GWMessageContext::Ptr context = m_contextPoll.remove(id);
				if (!context.isNull())
					m_outputQueue.enqueue(context);
			}
		);

		timedContext->setMissingResponseTask(task);
		m_contextPoll.insert(timedContext);

		try {
			sendMessage(timedContext->message());
		} catch (const Exception &e) {
			m_contextPoll.remove(id);
			e.rethrow();
		}

		m_timer.schedule(task, Timestamp() + m_resendTimeout);
	}
	else if (!context.isNull()) {
		sendMessage(context->message());
	}
}

void GWServerConnector::sendPing()
{
	FastMutex::ScopedLock guard(m_sendMutex);
	poco_trace(logger(), "sending ping frame");

	m_socket->sendFrame("echo", 4, WebSocket::FRAME_OP_PING);
}

Event &GWServerConnector::readyToSendEvent()
{
	return answerQueue().event();
}

void GWServerConnector::reconnect()
{
	FastMutex::ScopedLock sendGuard(m_sendMutex);
	FastMutex::ScopedLock receiveGuard(m_receiveMutex);
	disconnectUnlocked();
	connectAndRegisterUnlocked();
	m_isConnected = true;
	m_connectedEvent.set();
}

void GWServerConnector::startReceiver()
{
	m_receiverThread.startFunc([this](){
		runReceiver();
	});
}

void GWServerConnector::runReceiver()
{
	while (!m_stop) {
		if (!m_isConnected) {
			m_connectedEvent.wait();
			continue;
		}

		FastMutex::ScopedLock guard(m_receiveMutex);
		try {
			if (m_socket.isNull()) {
				throw ConnectionResetException(
					"server connection is not initialized");
			}

			if (!m_socket->poll(m_pollTimeout, Socket::SELECT_READ))
				continue;

			GWMessage::Ptr msg = receiveMessageUnlocked();
			if (!msg.isNull())
				handleMessage(msg);
		}
		catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
			markDisconnected();
		}
		catch (const exception &e) {
			logger().critical(e.what(), __FILE__, __LINE__);
			markDisconnected();
		}
		catch (...) {
			logger().critical("unknown error", __FILE__, __LINE__);
			markDisconnected();
		}
	}
}

void GWServerConnector::markDisconnected()
{
	m_isConnected = false;
}

void GWServerConnector::enqueueFinishedAnswers()
{
	FastMutex::ScopedLock guard(m_dispatchLock);
	list<Answer::Ptr> finishedAnswers = answerQueue().finishedAnswers();

	for (auto answer : finishedAnswers) {
		GWResponseWithAckContext::Ptr response;
		ServerAnswer::Ptr serverAnswer = answer.cast<ServerAnswer>();

		if (serverAnswer.isNull()) {
			logger().warning("expected instance of ServerAnswer", __FILE__, __LINE__);
			continue;
		}

		ServerAnswer::ScopedLock guard(const_cast<ServerAnswer &>(*serverAnswer));

		int failedResults = 0;

		for (size_t i = 0; i < serverAnswer->resultsCount(); i++) {
			auto result = serverAnswer->at(i);
			if (result->status() != Result::Status::SUCCESS)
				failedResults++;
		}

		GWResponse::Status status;
		if (failedResults > 0) {
			logger().warning(
				to_string(failedResults)
				+ "/" + to_string(serverAnswer->resultsCount())
				+ " results of answer "
				+ serverAnswer->id().toString()
				+ " has failed",
				__FILE__, __LINE__
			);
			status = GWResponse::Status::FAILED;
		}
		else if (serverAnswer->resultsCount() == 0) {
			logger().error("command was not accepted by anyone");
			status = GWResponse::Status::FAILED;
		}
		else {
			status = GWResponse::Status::SUCCESS;
		}

		response = serverAnswer->toResponse(status);
		m_outputQueue.enqueue(response);
		answerQueue().remove(answer);
	}
}

bool GWServerConnector::connectUnlocked()
{
	logger().information("connecting to server "
		+ m_host + ":" + to_string(m_port), __FILE__, __LINE__);

	try {
		HTTPRequest request(HTTPRequest::HTTP_1_1);
		HTTPResponse response;

		if (m_sslConfig.isNull()) {
			HTTPClientSession cs(m_host, m_port);
			m_socket = new WebSocket(cs, request, response);
		}
		else {
			HTTPSClientSession cs(m_host, m_port, m_sslConfig->context());
			m_socket = new WebSocket(cs, request, response);
		}

		m_socket->setReceiveTimeout(m_receiveTimeout);
		m_socket->setSendTimeout(m_sendTimeout);

		logger().information("successfully connected to server",
			__FILE__, __LINE__);
		return true;
	}
	catch (const Exception &e) {
		logger().log(e, __FILE__, __LINE__);
		return false;
	}
}

bool GWServerConnector::registerUnlocked()
{
	logger().information("registering gateway "
			+ m_gatewayInfo->gatewayID().toString(), __FILE__, __LINE__);

	try {
		GWGatewayRegister::Ptr registerMsg = new GWGatewayRegister;
		registerMsg->setGatewayID(m_gatewayInfo->gatewayID());
		registerMsg->setIPAddress(m_socket->address().host());
		registerMsg->setVersion(m_gatewayInfo->version());

		sendMessageUnlocked(registerMsg);

		GWMessage::Ptr msg = receiveMessageUnlocked();

		if (msg.cast<GWGatewayAccepted>().isNull()) {
			logger().error("unexpected response " + msg->type().toString(),
				__FILE__, __LINE__);
			return false;
		}

		logger().information("successfully registered", __FILE__, __LINE__);
		return true;
	}
	catch (const Exception &e) {
		logger().log(e, __FILE__, __LINE__);
		return false;
	}
}

void GWServerConnector::connectAndRegisterUnlocked()
{
	while (!m_stop) {
		try {
			if (connectUnlocked() && registerUnlocked())
				break;
		}
		catch (const exception &e) {
			logger().critical(e.what(), __FILE__, __LINE__);
		}
		catch (...) {
			logger().critical("unknown error", __FILE__, __LINE__);
		}

		if (m_stopEvent.tryWait(m_retryConnectTimeout.totalMilliseconds()))
			break;
	}
}

void GWServerConnector::disconnectUnlocked()
{
	if (!m_socket.isNull()) {
		m_socket = nullptr;

		logger().information("disconnected", __FILE__, __LINE__);
	}
}

void GWServerConnector::sendMessage(const GWMessage::Ptr message)
{
	FastMutex::ScopedLock guard(m_sendMutex);

	if (m_socket.isNull())
		throw ConnectionResetException("server connection is not initialized");

	sendMessageUnlocked(message);
}

void GWServerConnector::sendMessageUnlocked(const GWMessage::Ptr message)
{
	const string &msg = message->toString();

	if (logger().trace())
		logger().trace("send:\n" + msg, __FILE__, __LINE__);

	m_socket->sendFrame(msg.c_str(), msg.length());
}

GWMessage::Ptr GWServerConnector::receiveMessageUnlocked()
{
	int flags;
	int ret = m_socket->receiveFrame(m_receiveBuffer.begin(),
			m_receiveBuffer.size(), flags);
	const int opcode = flags & WebSocket::FRAME_OP_BITMASK;

	if (opcode == WebSocket::FRAME_OP_PONG) {
		poco_trace(logger(), "received pong message");
		updateLastReceived();
		return nullptr;
	}

	if (ret <= 0 || (opcode == WebSocket::FRAME_OP_CLOSE))
		throw ConnectionResetException("server connection closed");

	string data(m_receiveBuffer.begin(), ret);

	if (logger().trace())
		logger().trace("received:\n" + data, __FILE__, __LINE__);

	updateLastReceived();

	return GWMessage::fromJSON(data);
}

void GWServerConnector::setHost(const string &host)
{
	m_host = host;
}

void GWServerConnector::setPort(int port)
{
	if (port < 0 || port > 65535)
		throw InvalidArgumentException("port must be in range 0..65535");

	m_port = port;
}

void GWServerConnector::setPollTimeout(const Timespan &timeout)
{
	if (timeout < 0)
		throw InvalidArgumentException("receive timeout must be non negative");

	m_pollTimeout = timeout;
}

void GWServerConnector::setReceiveTimeout(const Timespan &timeout)
{
	if (timeout < 0)
		throw InvalidArgumentException("receive timeout must be non negative");

	m_receiveTimeout = timeout;
}

void GWServerConnector::setSendTimeout(const Timespan &timeout)
{
	if (timeout < 0)
		throw InvalidArgumentException("send timeout must be non negative");

	m_sendTimeout = timeout;
}

void GWServerConnector::setRetryConnectTimeout(const Timespan &timeout)
{
	if (timeout < 0)
		throw InvalidArgumentException("retryConnectTimeout must be non negative");

	m_retryConnectTimeout = timeout;
}

void GWServerConnector::setBusySleep(const Poco::Timespan &busySleep)
{
	if (busySleep < 0)
		throw InvalidArgumentException("retryConnectTimeout must be non negative");

	m_busySleep = busySleep;
}

void GWServerConnector::setResendTimeout(const Poco::Timespan &timeout)
{
	if (timeout < 0)
		throw InvalidArgumentException("retryConnectTimeout must be non negative");

	m_resendTimeout = timeout;
}

void GWServerConnector::setMaxMessageSize(int size)
{
	if (size <= 0)
		throw InvalidArgumentException("size must be non negative");

	m_maxMessageSize = size;
}

void GWServerConnector::setGatewayInfo(SharedPtr<GatewayInfo> info)
{
	m_gatewayInfo = info;
}

void GWServerConnector::setSSLConfig(SharedPtr<SSLClient> config)
{
	m_sslConfig = config;
}

void GWServerConnector::setInactiveMultiplier(int multiplier)
{
	if (multiplier < 1)
		throw InvalidArgumentException("multiplier must be greater than zero");
	m_inactiveMultiplier = multiplier;
}

bool GWServerConnector::ship(const SensorData &data)
{
	GWSensorDataExport::Ptr exportMessage = new GWSensorDataExport();
	GWSensorDataExportContext::Ptr exportContext = new GWSensorDataExportContext();

	GlobalID id = GlobalID::random();

	exportMessage->setID(id);
	exportMessage->setData({data});

	exportContext->setMessage(exportMessage);

	m_outputQueue.enqueue(exportContext);

	return true;
}

bool GWServerConnector::accept(const Command::Ptr cmd)
{
	return cmd->is<NewDeviceCommand>()
		|| cmd->is<ServerDeviceListCommand>()
		|| cmd->is<ServerLastValueCommand>();
}

void GWServerConnector::handle(Command::Ptr cmd, Answer::Ptr answer)
{
	if (cmd->is<NewDeviceCommand>())
		doNewDeviceCommand(cmd.cast<NewDeviceCommand>(), answer);
	else if (cmd->is<ServerDeviceListCommand>())
		doDeviceListCommand(cmd.cast<ServerDeviceListCommand>(), answer);
	else if (cmd->is<ServerLastValueCommand>())
		doLastValueCommand(cmd.cast<ServerLastValueCommand>(), answer);
	else
		throw IllegalStateException("received unexpected command");
}

void GWServerConnector::doNewDeviceCommand(NewDeviceCommand::Ptr cmd,
		Answer::Ptr answer)
{
	Result::Ptr result = new Result(answer);

	GlobalID id = GlobalID::random();

	GWNewDeviceRequest::Ptr request = new GWNewDeviceRequest;
	request->setID(id);
	request->setDeviceID(cmd->deviceID());
	request->setProductName(cmd->productName());
	request->setVendor(cmd->vendor());
	request->setRefreshTime(cmd->refreshTime());
	request->setModuleTypes(cmd->dataTypes());

	m_outputQueue.enqueue(new GWRequestContext(request, result));
}

void GWServerConnector::doDeviceListCommand(
		ServerDeviceListCommand::Ptr cmd, Answer::Ptr answer)
{
	ServerDeviceListResult::Ptr result = new ServerDeviceListResult(answer);

	GlobalID id = GlobalID::random();

	GWDeviceListRequest::Ptr request = new GWDeviceListRequest;
	request->setID(id);
	request->setDevicePrefix(cmd->devicePrefix());

	m_outputQueue.enqueue(new GWRequestContext(request, result));
}

void GWServerConnector::doLastValueCommand(
		ServerLastValueCommand::Ptr cmd, Answer::Ptr answer)
{
	ServerLastValueResult::Ptr result = new ServerLastValueResult(answer);
	result->setDeviceID(cmd->deviceID());
	result->setModuleID(cmd->moduleID());

	GlobalID id = GlobalID::random();

	GWLastValueRequest::Ptr request = new GWLastValueRequest;
	request->setID(id);
	request->setDeviceID(cmd->deviceID());
	request->setModuleID(cmd->moduleID());

	m_outputQueue.enqueue(new GWRequestContext(request, result));
}

void GWServerConnector::handleMessage(GWMessage::Ptr msg)
{
	if (!msg.cast<GWRequest>().isNull())
		handleRequest(msg.cast<GWRequest>());
	else if (!msg.cast<GWResponse>().isNull())
		handleResponse(msg.cast<GWResponse>());
	else if (!msg.cast<GWAck>().isNull())
		handleAck(msg.cast<GWAck>());
	else if (!msg.cast<GWSensorDataConfirm>().isNull())
		handleSensorDataConfirm(msg.cast<GWSensorDataConfirm>());
	else
		throw InvalidArgumentException("bad message type " + msg->type().toString());
}

void GWServerConnector::handleRequest(GWRequest::Ptr request)
{
	switch (request->type()) {
	case GWMessageType::DEVICE_ACCEPT_REQUEST:
		handleDeviceAcceptRequest(request.cast<GWDeviceAcceptRequest>());
		break;
	case GWMessageType::LISTEN_REQUEST:
		handleListenRequest(request.cast<GWListenRequest>());
		break;
	case GWMessageType::SET_VALUE_REQUEST:
		handleSetValueRequest(request.cast<GWSetValueRequest>());
		break;
	case GWMessageType::UNPAIR_REQUEST:
		handleUnpairRequest(request.cast<GWUnpairRequest>());
		break;
	default:
		throw InvalidArgumentException("bad request type "+ request->type().toString());
	}
}

void GWServerConnector::handleDeviceAcceptRequest(
	GWDeviceAcceptRequest::Ptr request)
{
	DeviceAcceptCommand::Ptr command = new DeviceAcceptCommand(request->deviceID());

	dispatchServerCommand(command, request->id(), request->derive());
}

void GWServerConnector::handleListenRequest(GWListenRequest::Ptr request)
{
	GatewayListenCommand::Ptr command =
		new GatewayListenCommand(request->duration());

	dispatchServerCommand(command, request->id(), request->derive());
}

void GWServerConnector::handleSetValueRequest(
	GWSetValueRequest::Ptr request)
{
	DeviceSetValueCommand::Ptr command =
		new DeviceSetValueCommand(
			request->deviceID(),
			request->moduleID(),
			request->value(),
			request->timeout());

	dispatchServerCommand(command, request->id(), request->derive());
}

void GWServerConnector::handleUnpairRequest(GWUnpairRequest::Ptr request)
{
	DeviceUnpairCommand::Ptr command =
		new DeviceUnpairCommand(request->deviceID());

	dispatchServerCommand(command, request->id(), request->derive());
}

void GWServerConnector::dispatchServerCommand(
		Command::Ptr cmd,
		const GlobalID &id,
		GWResponse::Ptr response)
{
	FastMutex::ScopedLock guard(m_dispatchLock);
	response->setStatus(GWResponse::Status::ACCEPTED);
	GWResponseContext::Ptr context = new GWResponseContext(response);
	m_outputQueue.enqueue(context);
	dispatch(cmd, new ServerAnswer(answerQueue(), id));
}

void GWServerConnector::handleResponse(GWResponse::Ptr response)
{
	GWRequestContext::Ptr context = m_contextPoll.remove(response->id()).cast<GWRequestContext>();

	if (context.isNull()) {
		poco_warning(logger(), "no coresponding request found, dropping response of type "
				+ response->toString()
				+ " with id: " + response->id().toString());
		return;
	}

	auto result = context->result();

	if (response->status() == GWResponse::Status::SUCCESS) {
		switch (response->type()) {
		case GWMessageType::GENERIC_RESPONSE:
			break;
		case GWMessageType::DEVICE_LIST_RESPONSE:
		{
			ServerDeviceListResult::Ptr deviceListResult = result.cast<ServerDeviceListResult>();
			if (deviceListResult.isNull())
				throw IllegalStateException("request result do not match with response result");
			deviceListResult->setDeviceList(response.cast<GWDeviceListResponse>()->devices());
			break;
		}
		case GWMessageType::LAST_VALUE_RESPONSE:
		{
			ServerLastValueResult::Ptr lastValueResult = result.cast<ServerLastValueResult>();
			if (lastValueResult.isNull())
				throw IllegalStateException("request result do not match with response result");
			lastValueResult->setValue(response.cast<GWLastValueResponse>()->value());
			break;
		}
		default:
			result->setStatus(Result::Status::FAILED);
			throw InvalidArgumentException("bad response type "
				+ response->type().toString());
		}

		result->setStatus(Result::Status::SUCCESS);
	}
	else {
		result->setStatus(Result::Status::FAILED);
	}
}

void GWServerConnector::handleSensorDataConfirm(GWSensorDataConfirm::Ptr confirm)
{
	m_contextPoll.remove(confirm->id());
}

void GWServerConnector::handleAck(GWAck::Ptr ack)
{
	m_contextPoll.remove(ack->id());
}

bool GWServerConnector::connectionSeemsBroken()
{
	FastMutex::ScopedLock guard(m_lastReceivedMutex);
	return m_lastReceived.isElapsed(
			m_busySleep.totalMicroseconds()
			* m_inactiveMultiplier);
}

void GWServerConnector::updateLastReceived()
{
	FastMutex::ScopedLock guard(m_lastReceivedMutex);
	m_lastReceived.update();
}
