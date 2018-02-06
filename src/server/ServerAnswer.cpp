#include <string>

#include "server/ServerAnswer.h"

using namespace Poco;
using namespace BeeeOn;
using namespace std;

ServerAnswer::ServerAnswer(AnswerQueue &answerQueue, const GlobalID &id):
	Answer(answerQueue),
	m_id(id)
{
}

void ServerAnswer::setID(const GlobalID &id)
{
	ScopedLock guard(*this);
	m_id = id;
}

GlobalID ServerAnswer::id() const
{
	ScopedLock guard(const_cast<ServerAnswer &>(*this));
	return m_id;
}

GWResponseWithAckContext::Ptr ServerAnswer::toResponse(const GWResponse::Status &status) const
{
	GWResponseWithAckContext::Ptr context;

	GWResponseWithAck::Ptr response = new GWResponseWithAck;
	response->setID(id());
	response->setStatus(status);

	context = new GWResponseWithAckContext(status);
	context->setMessage(response);

	return context;
}
