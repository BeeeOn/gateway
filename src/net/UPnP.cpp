#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/RegularExpression.h>

#include "net/UPnP.h"

#define BUFFER_LENGTH 256

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Net;
using namespace std;

UPnP::UPnP(const SocketAddress& address)
{
	m_multicastAddress = address;
}

list<SocketAddress> UPnP::discover(const Timespan& timeout, const string& deviceType)
{
	DatagramSocket socket(m_multicastAddress.family());
	socket.setBroadcast(true);

	const string msg = "M-SEARCH * HTTP/1.1\r\n"
	                   "HOST: " + m_multicastAddress.toString() + "\r\n"
	                   "MAN: \"ssdp:discover\"\r\n"
	                   "MX: " + to_string(timeout.totalSeconds()) + "\r\n"
	                   "ST: " + deviceType + "\r\n\r\n";

	socket.sendTo(msg.data(), msg.size(), m_multicastAddress);

	if (logger().debug()) {
		logger().debug("sent " + to_string(msg.size()) + " bytes",
			__FILE__, __LINE__);
	}

	socket.setReceiveTimeout(timeout);
	char buffer[BUFFER_LENGTH];
	SocketAddress device;
	list<SocketAddress> listOfDevices;

	logger().information("starting to looking for devices " + deviceType, __FILE__, __LINE__);

	while (true) {
		int sizeOfResponse;

		try {
			sizeOfResponse = socket.receiveFrom(buffer, sizeof(buffer), device);
		}
		catch (const TimeoutException& e) {
			break;
		}

		const string response = string(buffer, sizeOfResponse) + "\r\n";

		RegularExpression::MatchVec matches;
		RegularExpression reST("\r\n(?i)(st): (.*)\r\n");

		if (reST.match(response, 0, matches)) {
			if (response.substr(matches[2].offset, matches[2].length) != deviceType) {
				continue;
			}
		}

		RegularExpression reLocation(
			"(?i)(location): "
			"http://([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}:[0-9]{1,5})"
			"/[a-zA-z]*\\.xml\r\n");

		if (reLocation.match(response, 0, matches) == 0)
			continue;

		SocketAddress searchingDevice(response.substr(matches[2].offset, matches[2].length));

		auto it = find(listOfDevices.begin(), listOfDevices.end(), searchingDevice);
		if (it == listOfDevices.end())
			listOfDevices.push_front(searchingDevice);
	}

	logger().information("found " + to_string(listOfDevices.size()) + " device(s) " + deviceType, __FILE__, __LINE__);

	if (logger().debug()) {
		for (const auto &address : listOfDevices)
			logger().debug("found device at " + address.toString());
	}
	
	return listOfDevices;
}
