#include <Poco/Exception.h>

#include "server/GWMessageContext.h"

using namespace BeeeOn;

GWMessageContext::GWMessageContext(GWMessagePriority priority):
	m_priority(priority)
{
}

GWMessageContext::~GWMessageContext()
{
}

GWMessage::Ptr GWMessageContext::message()
{
	return m_message;
}

void GWMessageContext::setMessage(GWMessage::Ptr msg)
{
	m_message = msg;
}

int GWMessageContext::priority() const
{
	return m_priority;
}

GlobalID GWMessageContext::id() const
{
	if (m_message.isNull())
		throw Poco::IllegalStateException("message not set");

	return m_message->id();
}

GWTimedContext::GWTimedContext(GWMessagePriority priority):
	GWMessageContext(priority)
{
}

LambdaTimerTask::Ptr GWTimedContext::missingResponseTask()
{
	return m_missingResponseTask;
}

void GWTimedContext::setMissingResponseTask(LambdaTimerTask::Ptr task)
{
	m_missingResponseTask = task;
}

GWRequestContext::GWRequestContext():
	GWTimedContext(REQUEST_PRIO)
{
}

GWRequestContext::GWRequestContext(GWRequest::Ptr request, Result::Ptr result):
	GWTimedContext(REQUEST_PRIO),
	m_result(result)
{
	setMessage(request);
}

Result::Ptr GWRequestContext::result()
{
	return m_result;
}

void GWRequestContext::setResult(Result::Ptr result)
{
	m_result = result;
}
GWResponseContext::GWResponseContext():
	GWMessageContext(RESPONSE_PRIO)
{
}

GWResponseContext::GWResponseContext(GWResponse::Ptr response):
	GWMessageContext(RESPONSE_PRIO)
{
	setMessage(response);
}

GWResponseWithAckContext::GWResponseWithAckContext(GWResponse::Status status):
	GWTimedContext(RESPONSEWITHACK_PRIO),
	m_status(status)
{
}

GWResponse::Status GWResponseWithAckContext::status()
{
	return m_status;
}

void GWResponseWithAckContext::setStatus(GWResponse::Status status)
{
	m_status = status;
}

GWSensorDataExportContext::GWSensorDataExportContext():
	GWTimedContext(DATA_PRIO)
{
}

bool ContextPriorityComparator::operator()(const GWMessageContext::Ptr a, const GWMessageContext::Ptr b)
{
	return a->priority() < b->priority();
}
