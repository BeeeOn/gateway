#include <Poco/Logger.h>

#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceSearchCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"
#include "server/GWSCommandSender.h"

BEEEON_OBJECT_BEGIN(BeeeOn, GWSCommandSender)
BEEEON_OBJECT_CASTABLE(GWSListener)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &GWSCommandSender::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("connector", &GWSCommandSender::setConnector)
BEEEON_OBJECT_PROPERTY("unpairDuration", &GWSCommandSender::setUnpairDuration)
BEEEON_OBJECT_END(BeeeOn, GWSCommandSender)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

GWSCommandSender::RequestAnswer::RequestAnswer(
		AnswerQueue &queue,
		GWRequest::Ptr request):
	Answer(queue),
	m_request(request)
{
}

GWRequest::Ptr GWSCommandSender::RequestAnswer::request() const
{
	return m_request;
}

GWSCommandSender::GWSCommandSender():
	m_unpairDuration(5 * Timespan::SECONDS)
{
}

void GWSCommandSender::setConnector(GWSConnector::Ptr connector)
{
	m_connector = connector;
}

void GWSCommandSender::setUnpairDuration(const Timespan &duration)
{
	if (duration < 1 * Timespan::SECONDS)
		throw InvalidArgumentException("unpairDuration must be at least 1 s");

	m_unpairDuration = duration;
}

void GWSCommandSender::onRequest(GWRequest::Ptr request)
{
	switch (request->type()) {
	case GWMessageType::DEVICE_ACCEPT_REQUEST:
		handleDeviceAccept(request.cast<GWDeviceAcceptRequest>());
		break;
	case GWMessageType::LISTEN_REQUEST:
		handleListen(request.cast<GWListenRequest>());
		break;
	case GWMessageType::SEARCH_REQUEST:
		handleSearch(request.cast<GWSearchRequest>());
		break;
	case GWMessageType::SET_VALUE_REQUEST:
		handleSetValue(request.cast<GWSetValueRequest>());
		break;
	case GWMessageType::UNPAIR_REQUEST:
		handleUnpair(request.cast<GWUnpairRequest>());
		break;
	default:
		logger().warning(
			"unhandled request " + request->toBriefString(),
			__FILE__, __LINE__);
		return;
	}
}

void GWSCommandSender::run()
{
	StopControl::Run run(m_stopControl);

	logger().information("starting GWS command sender");

	while (run) {
		list<Answer::Ptr> dirty;
		answerQueue().wait(-1, dirty);

		if (logger().debug() && dirty.empty()) {
			logger().debug(
				"processing " + to_string(dirty.size()) + " answers",
				__FILE__, __LINE__);
		}

		for (auto answer : dirty) {
			Answer::ScopedLock guard(*answer);

			if (answer->isPending())
				continue;
	
			bool failed = false;

			for (auto result : *answer) {
				if (result->status() == Result::Status::FAILED)
					failed = true;
			}

			try {
				respond(answer.cast<RequestAnswer>(), failed);
			}
			BEEEON_CATCH_CHAIN(logger())

			answerQueue().remove(answer);
		}
	}

	logger().information("GWS command sender has stopped");
}

void GWSCommandSender::respond(RequestAnswer::Ptr answer, bool failed)
{
	GWResponse::Ptr response = answer->request()->derive();

	if (failed)
		response->setStatus(GWResponse::Status::FAILED);
	else
		response->setStatus(GWResponse::Status::SUCCESS);

	m_connector->send(response);
}

void GWSCommandSender::stop()
{
	m_stopControl.requestStop();
	answerQueue().dispose();
	answerQueue().event().set();
}

void GWSCommandSender::handleDeviceAccept(GWDeviceAcceptRequest::Ptr request)
{
	DeviceAcceptCommand::Ptr command = new DeviceAcceptCommand(request->deviceID());
	dispatch(command, request);
}

void GWSCommandSender::handleListen(GWListenRequest::Ptr request)
{
	GatewayListenCommand::Ptr command = new GatewayListenCommand(request->duration());
	dispatch(command, request);
}

void GWSCommandSender::handleSearch(GWSearchRequest::Ptr request)
{
	DeviceSearchCommand::Ptr command = new DeviceSearchCommand(
		request->devicePrefix(),
		request->criteria(),
		request->duration());
	dispatch(command, request);
}

void GWSCommandSender::handleSetValue(GWSetValueRequest::Ptr request)
{
	DeviceSetValueCommand::Ptr command = new DeviceSetValueCommand(
		request->deviceID(),
		request->moduleID(),
		request->value(),
		request->timeout());

	dispatch(command, request);
}

void GWSCommandSender::handleUnpair(GWUnpairRequest::Ptr request)
{
	DeviceUnpairCommand::Ptr command = new DeviceUnpairCommand(
		request->deviceID(),
		m_unpairDuration);

	dispatch(command, request);
}

void GWSCommandSender::dispatch(Command::Ptr command, GWRequest::Ptr request)
{
	if (logger().debug()) {
		logger().debug(
			"request " + request->toBriefString()
			+ " accepted, dispatching " + command->toString(),
			__FILE__, __LINE__);
	}

	GWResponse::Ptr response = new GWResponse; // no acking here
	response->setID(request->id());
	response->setStatus(GWResponse::Status::ACCEPTED);
	m_connector->send(response);

	RequestAnswer::Ptr answer = new RequestAnswer(answerQueue(), request);
	CommandSender::dispatch(command, answer);
}
