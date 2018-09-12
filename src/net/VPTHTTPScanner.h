#pragma once

#include <Poco/Net/IPAddress.h>
#include <Poco/Net/SocketAddress.h>

#include "net/AbstractHTTPScanner.h"

namespace BeeeOn {

class VPTHTTPScanner : public AbstractHTTPScanner {
public:
	VPTHTTPScanner();
	VPTHTTPScanner(const std::string& path, uint16_t port, const Poco::Net::IPAddress& minNetMask);
	~VPTHTTPScanner();

protected:
	void prepareRequest(Poco::Net::HTTPRequest& request) override;
	bool isValidResponse(const std::string& response) override;
};

}
