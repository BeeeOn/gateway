#include "di/Injectable.h"
#include "net/GatewayMosquittoClient.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

BEEEON_OBJECT_BEGIN(BeeeOn, GatewayMosquittoClient)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(MosquittoClient)
BEEEON_OBJECT_PROPERTY("port", &GatewayMosquittoClient::setPort)
BEEEON_OBJECT_PROPERTY("host", &GatewayMosquittoClient::setHost)
BEEEON_OBJECT_PROPERTY("clientID", &GatewayMosquittoClient::setClientID)
BEEEON_OBJECT_PROPERTY("reconnectTimeout", &GatewayMosquittoClient::setReconnectTimeout)
BEEEON_OBJECT_PROPERTY("subTopics", &GatewayMosquittoClient::setSubTopics)
BEEEON_OBJECT_PROPERTY("gatewayInfo", &GatewayMosquittoClient::setGatewayInfo)
BEEEON_OBJECT_END(BeeeOn, GatewayMosquittoClient)

void GatewayMosquittoClient::setGatewayInfo(SharedPtr<GatewayInfo> info)
{
	m_gatewayInfo = info;
}

string GatewayMosquittoClient::buildClientID() const
{
	if (clientID().empty() && !m_gatewayInfo.isNull())
		return m_gatewayInfo->gatewayID().toString();

	if (!clientID().empty() && m_gatewayInfo.isNull())
		return clientID();

	if (!clientID().empty() && !m_gatewayInfo.isNull())
		return m_gatewayInfo->gatewayID().toString() + "_" + clientID();

	throw IllegalStateException("client id nor gateway info is not set");
}
