#pragma once

#include <Poco/AutoPtr.h>

#include "core/Answer.h"
#include "model/GlobalID.h"
#include "server/GWMessageContext.h"

namespace BeeeOn {

/**
 * @brief ServerAnswer extends regular Answer with ID of
 * corresponding received GWMessage, which was translated to dispatched
 * Command. This ID is used to create GWMessage to inform server
 * about execution status of this Command.
 */
class ServerAnswer : public Answer {
public:
	typedef Poco::AutoPtr<ServerAnswer> Ptr;

	ServerAnswer(AnswerQueue &answerQueue, const GlobalID &id);

	ServerAnswer(const ServerAnswer&) = delete;

	void setID(const GlobalID &id);

	/**
	 * Convert Answer to appropriate GWResponseWithAckContext if possible.
	 */
	GWResponseWithAckContext::Ptr toResponse(const GWResponse::Status &status) const;

	GlobalID id() const;

private:
	GlobalID m_id;
};

}
