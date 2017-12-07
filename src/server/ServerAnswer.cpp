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
	FastMutex::ScopedLock guard(lock());
	setIDUnlocked(id);
}

void ServerAnswer::setIDUnlocked(const GlobalID &id)
{
	assureLocked();
	m_id = id;
}

GlobalID ServerAnswer::id() const
{
	FastMutex::ScopedLock guard(lock());
	return idUnlocked();
}

GlobalID ServerAnswer::idUnlocked() const
{
	assureLocked();
	return m_id;
}

GWResponseWithAckContext::Ptr ServerAnswer::toResponse(const GWResponse::Status &status) const
{
	GWResponseWithAckContext::Ptr context;

	GWResponseWithAck::Ptr response = new GWResponseWithAck;
	response->setID(idUnlocked());
	response->setStatus(status);

	context = new GWResponseWithAckContext(status);
	context->setMessage(response);

	return context;
}
