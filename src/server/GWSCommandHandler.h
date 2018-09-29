#pragma once

#include <map>

#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>

#include "commands/NewDeviceCommand.h"
#include "commands/ServerDeviceListCommand.h"
#include "commands/ServerLastValueCommand.h"
#include "core/CommandHandler.h"
#include "gwmessage/GWDeviceListResponse.h"
#include "gwmessage/GWLastValueResponse.h"
#include "server/GWSConnector.h"
#include "server/GWSListener.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief Handle requests to the remote server:
 * - NewDeviceCommand
 * - ServerDeviceListCommand
 * - ServerLastValueCommand
 *
 * The commands are converted to the appropriate GWMessage representations
 * and sent to the remote server. Received responses are used to update the
 * associated Answer and Result instances.
 */
class GWSCommandHandler :
	public CommandHandler,
	public GWSListener,
	Loggable {
public:
	typedef Poco::SharedPtr<GWSCommandHandler> Ptr;

	GWSCommandHandler();

	void setConnector(GWSConnector::Ptr connector);

	bool accept(const Command::Ptr cmd) override;
	void handle(Command::Ptr cmd, Answer::Ptr answer) override;

	/**
	 * @brief Check whether the responses associated with a pending command.
	 * If it is, update its result according to the response contents.
	 */
	void onResponse(const GWResponse::Ptr response) override;

protected:
	void handleNewDevice(NewDeviceCommand::Ptr cmd, Answer::Ptr answer);
	void handleDeviceList(ServerDeviceListCommand::Ptr cmd, Answer::Ptr answer);
	void handleLastValue(ServerLastValueCommand::Ptr cmd, Answer::Ptr answer);
	void sendRequest(GWRequest::Ptr request, Result::Ptr answer);
	void sendRequestUnsafe(GWRequest::Ptr request, Result::Ptr answer);

	void onSpecificResponse(const GWResponse::Ptr response, Result::Ptr result);
	void onDeviceListResponse(const GWDeviceListResponse::Ptr response, Result::Ptr result);
	void onLastValueResponse(const GWLastValueResponse::Ptr response, Result::Ptr result);

private:
	GWSConnector::Ptr m_connector;
	std::map<GlobalID, Result::Ptr> m_pending;
	Poco::FastMutex m_lock;
};

}
