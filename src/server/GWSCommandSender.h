#pragma once

#include <Poco/Timespan.h>

#include "core/CommandSender.h"
#include "gwmessage/GWDeviceAcceptRequest.h"
#include "gwmessage/GWListenRequest.h"
#include "gwmessage/GWSetValueRequest.h"
#include "gwmessage/GWUnpairRequest.h"
#include "loop/StoppableRunnable.h"
#include "loop/StopControl.h"
#include "server/GWSConnector.h"
#include "server/GWSListener.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief Pass requests received from the remote server
 * to the configured CommandDispatcher instance. The following
 * messages are processed:
 * - GWDeviceAcceptRequest
 * - GWListenRequest
 * - GWSetValueRequest
 * - GWUnpairRequest
 *
 * Changes of Answer and Result are translated back to appropriate
 * GWMessage instances and send to the remote server.
 */
class GWSCommandSender :
	public StoppableRunnable,
	public GWSListener,
	public CommandSender,
	Loggable {
public:
	GWSCommandSender();

	void setConnector(GWSConnector::Ptr connector);
	void setUnpairDuration(const Poco::Timespan &duration);

	/**
	 * @brief A loop that processes answers' updates of commands
	 * as created from the remote server requests.
	 */
	void run() override;
	void stop() override;

	/**
	 * @brief Receive a request from the remote server, convert it
	 * to the appropriate Command instance and dispatch to the rest
	 * of the application.
	 */
	void onRequest(GWRequest::Ptr request) override;

protected:
	void handleDeviceAccept(GWDeviceAcceptRequest::Ptr request);
	void handleListen(GWListenRequest::Ptr request);
	void handleSetValue(GWSetValueRequest::Ptr request);
	void handleUnpair(GWUnpairRequest::Ptr request);
	void dispatch(Command::Ptr command, GWRequest::Ptr request);

	class RequestAnswer : public Answer {
	public:
		typedef Poco::AutoPtr<RequestAnswer> Ptr;

		RequestAnswer(AnswerQueue &queue, GWRequest::Ptr request);

		GWRequest::Ptr request() const;

	private:
		GWRequest::Ptr m_request;
	};

	/**
	 * @brief When a RequestAnswer instance becomes finished, report
	 * its results to the remote server.
	 */
	void respond(RequestAnswer::Ptr answer, bool failed);

private:
	Poco::Timespan m_unpairDuration;
	GWSConnector::Ptr m_connector;
	StopControl m_stopControl;
};

}
