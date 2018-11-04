#pragma once

#include <list>
#include <string>

#include <Poco/Net/SocketAddress.h>
#include <Poco/Timespan.h>

#include "util/Loggable.h"

// Documentation for UPnP protocol
// http://upnp.org/specs/arch/UPnP-arch-DeviceArchitecture-v1.1.pdf
// Section 1 Discovery
#define UPNP_MULTICAST_IP "239.255.255.250"
#define UPNP_PORT 1900

namespace BeeeOn {

class UPnP : protected Loggable {
public:
	UPnP(const Poco::Net::SocketAddress& address = Poco::Net::SocketAddress(UPNP_MULTICAST_IP, UPNP_PORT));
	std::list<Poco::Net::SocketAddress> discover(const Poco::Timespan& timeout, const std::string& deviceType);

private:
	Poco::Net::SocketAddress m_multicastAddress;
};

}
