#ifndef GATEWAY_VPT_HTTP_SCANNER_H
#define GATEWAY_VPT_HTTP_SCANNER_H

#include <Poco/Net/IPAddress.h>
#include <Poco/Net/SocketAddress.h>

#include "net/AbstractHTTPScanner.h"

namespace BeeeOn {

class VPTHTTPScanner : public AbstractHTTPScanner {
public:
	VPTHTTPScanner(const std::string& path, Poco::UInt16 port, const Poco::Net::IPAddress& minNetMask);
	~VPTHTTPScanner();

protected:
	void prepareRequest(Poco::Net::HTTPRequest& request) override;
	bool isValidResponse(const std::string& response) override;
};

}

#endif
