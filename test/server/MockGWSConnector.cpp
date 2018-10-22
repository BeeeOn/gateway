#include "server/MockGWSConnector.h"

using namespace Poco;
using namespace BeeeOn;

void MockGWSConnector::setSendException(const SharedPtr<Exception> e)
{
	m_sendException = e;
}

void MockGWSConnector::send(const GWMessage::Ptr message)
{
	fireEvent(message, &GWSListener::onTrySend);

	if (!m_sendException.isNull())
		throw *m_sendException;

	fireEvent(message, &GWSListener::onSent);
}

void MockGWSConnector::receive(const GWMessage::Ptr message)
{
	fireReceived(message);
}
