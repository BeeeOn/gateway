#ifndef GATEWAY_ABSTRACT_HTTP_SCANNER_H
#define GATEWAY_ABSTRACT_HTTP_SCANNER_H

#include <set>

#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/IPAddress.h>
#include <Poco/Net/NetworkInterface.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Timespan.h>

#include "net/HTTPEntireResponse.h"
#include "net/IPAddressRange.h"
#include "util/Loggable.h"

namespace BeeeOn {

/*
 * @brief The abstract class contains core of HTTP scanning
 * of network. Derivated classes will have to implement
 * methods to prepare HTTP request and if the response
 * is from right device.
 */
class AbstractHTTPScanner : protected Loggable {
public:
	AbstractHTTPScanner();
	/*
	 * @param path Defines path of HTTP request.
	 * @param port Defines receiver's port.
	 * @param minNetMask Defines minimal range of exploring IP address.
	 */
	AbstractHTTPScanner(const std::string& path, uint16_t port, const Poco::Net::IPAddress& minNetMask);
	~AbstractHTTPScanner();

	/*
	 * @brief It executes the scan of proper network interfaces.
	 * @param maxResponseLength Defines maximal length of response message which
	 * will be process.
	 * @return Vector of SocketAddress belongs to found devices.
	 */
	std::vector<Poco::Net::SocketAddress> scan(const uint32_t maxResponseLength);

	void setPath(const std::string& path);
	void setPort(Poco::UInt16 port);
	void setMinNetMask(const Poco::Net::IPAddress& minNetMask);
	void setPingTimeout(const Poco::Timespan& pingTimeout);
	void setHTTPTimeout(const Poco::Timespan& httpTimeout);
	void setBlackList(const std::set<std::string>& set);

	std::string path();
	Poco::UInt16 port();

protected:
	virtual void prepareRequest(Poco::Net::HTTPRequest& request) = 0;
	/*
	 * @brief It defines if the response is valid or not.
	 * @param response Response message.
	 * @return If the response is valid or not.
	 */
	virtual bool isValidResponse(const std::string& response) = 0;

	/*
	 * @brief It explores network interface.
	 * @param interafce Exploring interface.
	 * @param devices Structure where are found devices saved.
	 * @param maxResponseLength Defines maximal length of response message which
	 * will be process.
	 */
	void probeInterface(const Poco::Net::NetworkInterface& interface,
		std::vector<Poco::Net::SocketAddress>& devices,
		const Poco::Int64 maxResponseLength);

	/*
	 * @brief It explores IP address range.
	 * @param range Exploring IP address range.
	 * @param devices Structure where are found devices saved.
	 * @param maxResponseLength Defines maximal length of response message which
	 * will be process.
	 */
	void probeAddressRange(const IPAddressRange& range,
		std::vector<Poco::Net::SocketAddress>& devices,
		const Poco::Int64 maxResponseLength);

	/*
	 * @brief It sends HTTP request.
	 * @param socketAddress Address of receiver.
	 * @param maxResponseLength Defines maximal length of response message which
	 * will be process.
	 * @return HTTP response.
	 */
	HTTPEntireResponse sendRequest(const Poco::Net::SocketAddress& socketAddress,
		const Poco::Int64 maxResponseLength);

	bool addressIncompatible(const Poco::Net::NetworkInterface::AddressTuple& addressTuple) const;

	/*
	 * @return Vector of active network interfaces.
	 */
	std::vector<Poco::Net::NetworkInterface> listNetworkInterfaces();

private:
	std::string m_path;
	uint16_t m_port;
	Poco::Net::IPAddress m_minNetMask;
	Poco::Timespan m_pingTimeout;
	Poco::Timespan m_httpTimeout;
	std::set<std::string> m_blackList;
};

}

#endif
