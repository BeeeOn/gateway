#include "gwmessage/GWAck.h"
#include "gwmessage/GWRequest.h"
#include "gwmessage/GWResponse.h"
#include "server/GWSConnector.h"

using namespace BeeeOn;

GWSConnector::~GWSConnector()
{
}

void GWSConnector::fireReceived(const GWMessage::Ptr message)
{
	const GWRequest::Ptr request = message.cast<GWRequest>();
	if (!request.isNull()) {
		fireEvent(request, &GWSListener::onRequest);
		return;
	}

	const GWResponse::Ptr response = message.cast<GWResponse>();
	if (!response.isNull()) {
		fireEvent(response, &GWSListener::onResponse);
		return;
	}

	const GWAck::Ptr ack = message.cast<GWAck>();
	if (!ack.isNull()) {
		fireEvent(ack, &GWSListener::onAck);
		return;
	}

	fireEvent(message, &GWSListener::onOther);
}


void GWSConnector::addListener(GWSListener::Ptr listener)
{
	m_eventSource.addListener(listener);
}

void GWSConnector::clearListeners()
{
	m_eventSource.clearListeners();
}

void GWSConnector::setEventsExecutor(AsyncExecutor::Ptr executor)
{
	m_eventSource.setAsyncExecutor(executor);
}
