#pragma once

#include <string>

#include <Poco/SharedPtr.h>

#include "core/GatewayInfo.h"
#include "net/MosquittoClient.h"

namespace BeeeOn {

/**
 * GatewayMosquittoClient generates client identification
 * on the basis of the client identification or gateway identification.
 * If both parameters are entered, these parameters are joined,
 * for example: gatewayID_clientID.
 */
class GatewayMosquittoClient : public MosquittoClient {
public:
	void setGatewayInfo(Poco::SharedPtr<GatewayInfo> info);

	std::string buildClientID() const override;

private:
	Poco::SharedPtr<GatewayInfo> m_gatewayInfo;
};

}
