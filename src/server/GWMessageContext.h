#pragma once

#include <Poco/SharedPtr.h>

#include "core/Result.h"
#include "gwmessage/GWRequest.h"
#include "gwmessage/GWResponse.h"
#include "gwmessage/GWResponseWithAck.h"
#include "gwmessage/GWSensorDataExport.h"
#include "model/GlobalID.h"
#include "util/LambdaTimerTask.h"

namespace BeeeOn {

/**
 * Priority for messages, message types with higher priority are defined
 * lower in this enum.
 */
enum GWMessagePriority {
	DEFAULT_PRIO = 0,
	REQUEST_PRIO = 10,
	RESPONSE_PRIO = 20,
	RESPONSEWITHACK_PRIO = 30,
	DATA_PRIO = 40,
};

/**
 * Encapsulate GWMessage with its priority for sending. Higher priority
 * means message is forwarded sooner.
 *
 * This is typically used for messages forwarded to server to hold an additional
 * information needed in case of response will arrive or, in case its needed, to resend
 * the message.
 *
 * This class is meant to be derived by specific message type context.
 */
class GWMessageContext {
public:
	typedef Poco::SharedPtr<GWMessageContext> Ptr;

	GWMessageContext(GWMessagePriority priority);

	virtual ~GWMessageContext();

	GWMessage::Ptr message();
	void setMessage(GWMessage::Ptr msg);

	int priority() const;

	GlobalID id() const;

protected:
	int m_priority;
	GWMessage::Ptr m_message;
};

/**
 * GWTimedContext extends base GWMessageContext with LambdaTimerTask.
 * This class is meant to be derived by specific message type
 * context, which expects response to be received in given time,
 * otherwise the given task is executed.
 */
class GWTimedContext : public GWMessageContext {
public:
	typedef Poco::SharedPtr<GWTimedContext> Ptr;

	GWTimedContext(GWMessagePriority priority);

	LambdaTimerTask::Ptr missingResponseTask();
	void setMissingResponseTask(LambdaTimerTask::Ptr task);

private:
	LambdaTimerTask::Ptr m_missingResponseTask;
};

/**
 * GWRequestContext contains Result::Ptr of the BeeeOn Command
 * executed by this request.
 */
class GWRequestContext : public GWTimedContext {
public:
	typedef Poco::SharedPtr<GWRequestContext> Ptr;

	GWRequestContext();
	GWRequestContext(GWRequest::Ptr request, Result::Ptr result);

	Result::Ptr result();
	void setResult(Result::Ptr result);
private:
	Result::Ptr m_result;
};

/**
 * GWResponseContext is used to store GWResponse message with GWResponse priority.
 */
class GWResponseContext : public GWMessageContext {
public:
	typedef Poco::SharedPtr<GWResponseContext> Ptr;

	GWResponseContext();
	GWResponseContext(GWResponse::Ptr response);
};

/**
 * GWResponseWithAckContext extends GWTimedContext with status of the
 * running task, which was invoked by by a matching GWRequest message.
 * Context of this type with status ACCEPTED has automatically higher priority,
 * as its expected to be sent before FAILED or SUCCESS status.
 */
class GWResponseWithAckContext : public GWTimedContext {
public:
	typedef Poco::SharedPtr<GWResponseWithAckContext> Ptr;

	GWResponseWithAckContext(GWResponse::Status status);

	GWResponse::Status status();
	void setStatus(GWResponse::Status);

private:
	GWResponse::Status m_status;
};

/**
 * GWSensorDataExportContext holds data message.
 */
class GWSensorDataExportContext : public GWTimedContext {
public:
	typedef Poco::SharedPtr<GWSensorDataExportContext> Ptr;

	GWSensorDataExportContext();
};

/**
 * Comparator for GWMessageContext, which can be usued to construct priority_queue of
 * GWMessageContext items, so the dequeue() operation always returns the highest priority message.
 */
class ContextPriorityComparator {
public:
	bool operator()(const GWMessageContext::Ptr a, const GWMessageContext::Ptr b);
};

}
