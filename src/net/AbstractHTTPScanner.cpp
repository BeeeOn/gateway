#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/SharedPtr.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>

#include "net/AbstractHTTPScanner.h"

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Net;
using namespace std;

AbstractHTTPScanner::AbstractHTTPScanner():
	m_port(0),
	m_minNetMask("255.255.255.255")
{
}

AbstractHTTPScanner::AbstractHTTPScanner(const string& path, uint16_t port, const IPAddress& minNetMask):
	m_path(path),
	m_port(port),
	m_minNetMask(minNetMask)
{
}

AbstractHTTPScanner::~AbstractHTTPScanner()
{
}

void AbstractHTTPScanner::setPath(const string& path)
{
	m_path = path;
}

void AbstractHTTPScanner::setPort(uint16_t port)
{
	m_port = port;
}

void AbstractHTTPScanner::setMinNetMask(const IPAddress& minNetMask)
{
	m_minNetMask = minNetMask;
}

void AbstractHTTPScanner::setPingTimeout(const Timespan& pingTimeout)
{
	m_pingTimeout = pingTimeout;
}

void AbstractHTTPScanner::setHTTPTimeout(const Timespan& httpTimeout)
{
	m_httpTimeout = httpTimeout;
}

void AbstractHTTPScanner::setBlackList(const set<string>& blackList)
{
	m_blackList = blackList;
}

string AbstractHTTPScanner::path()
{
	return m_path;
}

UInt16 AbstractHTTPScanner::port()
{
	return m_port;
}

vector<SocketAddress> AbstractHTTPScanner::scan(const uint32_t maxResponseLength)
{
	vector<NetworkInterface> listOfNetworkInterfaces = listNetworkInterfaces();
	vector<SocketAddress> devices;

	for (auto &interface : listOfNetworkInterfaces)
		probeInterface(interface, devices, maxResponseLength);

	if (devices.empty())
		logger().notice("no device found", __FILE__, __LINE__);

	return devices;
}

void AbstractHTTPScanner::probeInterface(const NetworkInterface& interface,
	vector<SocketAddress>& devices,
	const Int64 maxResponseLength)
{
	logger().notice("probing interface " + interface.adapterName(),
		__FILE__, __LINE__);

	for (auto &addressTuple : interface.addressList()) {
		if (addressIncompatible(addressTuple)) {
			logger().debug("incompatible address " + addressTuple.get<NetworkInterface::IP_ADDRESS>().toString());
			continue;
		}

		IPAddress networkAddress = addressTuple.get<NetworkInterface::IP_ADDRESS>();
		IPAddress netMask = addressTuple.get<NetworkInterface::SUBNET_MASK>();
		networkAddress.mask(netMask);

		if (netMask < m_minNetMask) {
			netMask = m_minNetMask;
			logger().warning("truncate scanning range to: " + m_minNetMask.toString(),
				__FILE__, __LINE__);
		}

		IPAddressRange range(networkAddress, netMask);
		probeAddressRange(range, devices, maxResponseLength);
	}
}

void AbstractHTTPScanner::probeAddressRange(const IPAddressRange& range,
	vector<SocketAddress>& devices,
	const Int64 maxResponseLength)
{
	StreamSocket ping;
	IPAddress ip;

	for (auto& ip : range) {
		SocketAddress socketAddress(ip, m_port);

		try {
			ping.connect(socketAddress, m_pingTimeout);
		}
		catch (Exception& e) {
			ping.close();
			continue;
		}

		ping.close();
		logger().debug("service detected at " + ip.toString() + ":" + to_string(m_port));

		HTTPEntireResponse response;
		try {
			response = sendRequest(socketAddress, maxResponseLength);
		}
		catch (TimeoutException& e) {
			logger().debug("timeout expired", __FILE__, __LINE__);
			continue;
		}
		catch (Exception& e) {
			if (logger().debug())
				logger().log(e, __FILE__, __LINE__);
			continue;
		}

		if (response.getStatus() != 200) {
			logger().warning("drop response " + to_string(response.getStatus()), __FILE__, __LINE__);
			continue;
		}

		if (isValidResponse(response.getBody()))
			devices.push_back(socketAddress);
	}
}

HTTPEntireResponse AbstractHTTPScanner::sendRequest(const SocketAddress& socketAddress, const Int64 maxResponseLength)
{
	HTTPClientSession http;
	HTTPEntireResponse response;
	HTTPRequest request;

	http.setHost(socketAddress.host().toString());
	http.setPort(socketAddress.port());
	http.setTimeout(m_httpTimeout);

	prepareRequest(request);

	logger().information("request: " + socketAddress.toString() + request.getURI(), __FILE__, __LINE__);

	http.sendRequest(request);

	istream &input = http.receiveResponse(response);

	if (response.getContentLength64() > maxResponseLength)
		throw RangeException("too long response");

	response.readBody(input);
	const int status = response.getStatus();
	if (status >= 400)
		logger().warning("response: " + to_string(status), __FILE__, __LINE__);
	else
		logger().information("response: " + to_string(status), __FILE__, __LINE__);

	return response;
}

bool AbstractHTTPScanner::addressIncompatible(const NetworkInterface::AddressTuple& addressTuple) const
{
	const auto &family = addressTuple.get<NetworkInterface::IP_ADDRESS>().family();
	return family != IPAddress::Family::IPv4 || addressTuple.length != 3;
}

vector<NetworkInterface> AbstractHTTPScanner::listNetworkInterfaces()
{
	vector<NetworkInterface> list;
	NetworkInterface::NetworkInterfaceList listOfNetworkInterfaces = NetworkInterface::list();

	for (auto &interface : listOfNetworkInterfaces) {
		if (interface.isPointToPoint() || interface.isLoopback()) {
			logger().debug("auto skipping interface " + interface.adapterName());
			continue;
		}

		if (m_blackList.find(interface.adapterName()) != m_blackList.end()) {
			logger().information("skipping blacklisted interface " + interface.adapterName());
			continue;
		}

		list.push_back(interface);
	}

	if (list.empty())
		logger().warning("no valid interface found", __FILE__, __LINE__);

	return list;
}
