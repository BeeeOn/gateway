#include "server/GWSListener.h"

using namespace std;
using namespace Poco::Net;
using namespace BeeeOn;

string GWSListener::Address::toString() const
{
	return host + ":" + to_string(port);
}

GWSListener::~GWSListener()
{
}

void GWSListener::onConnected(const Address &)
{
}

void GWSListener::onDisconnected(const Address &)
{
}

void GWSListener::onRequest(const GWRequest::Ptr)
{
}

void GWSListener::onResponse(const GWResponse::Ptr)
{
}

void GWSListener::onAck(const GWAck::Ptr)
{
}

void GWSListener::onOther(const GWMessage::Ptr)
{
}

void GWSListener::onTrySend(const GWMessage::Ptr)
{
}

void GWSListener::onSent(const GWMessage::Ptr)
{
}
