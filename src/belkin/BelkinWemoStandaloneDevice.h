#pragma once

#include <string>

#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/URI.h>
#include <Poco/Net/SocketAddress.h>

#include "belkin/BelkinWemoDevice.h"
#include "net/HTTPEntireResponse.h"
#include "net/MACAddress.h"

namespace BeeeOn {

/**
 * @brief Abstract class representing generic BelkinWemo standalone device.
 * The class implements sending messages to control the device.
 */
class BelkinWemoStandaloneDevice : public BelkinWemoDevice {
public:
	typedef Poco::SharedPtr<BelkinWemoStandaloneDevice> Ptr;

	BelkinWemoStandaloneDevice(
		const Poco::URI& uri,
		const Poco::Timespan &httpTimeout,
		const RefreshTime &refresh);

	/**
	 * @brief Prepares SOAP message containing GetBinaryState request
	 * and sends it to device via HTTP. If the device do not respond
	 * in specified timeout, Poco::TimeoutException is thrown.
	 */
	HTTPEntireResponse requestBinaryState() const;

	/**
	 * @brief Prepares SOAP message containing SetBinaryState request
	 * and sends it to device via HTTP. If the device do not respond
	 * in specified timeout, Poco::TimeoutException is thrown.
	 * @param &setModuleName name of XML element used in sent message.
	 * @param &getModuleName name of XML element used for searching in received message.
	 * @return If the request was successful or not.
	 */
	bool requestModifyBinaryState(const std::string& setModuleName,
		const std::string& getModuleName, const int value) const;

	Poco::Net::SocketAddress address() const;
	void setAddress(const Poco::Net::SocketAddress& address);

private:
	/**
	 * @brief Prepares SOAP message containing GetMacAddr request
	 * and sends it to device via HTTP. If the device do not respond
	 * in specified timeout, Poco::TimeoutException is thrown.
	 * @return MAC address of the device.
	 */
	static MACAddress requestMacAddr(
		const Poco::URI &uri,
		const Poco::Timespan &httpTimeout);

	/**
	 * @brief Called internally when constructing the instance.
	 * Creates DeviceID based on Mac address of device.
	 */
	static DeviceID buildDeviceID(
		const Poco::URI& uri,
		const Poco::Timespan& httpTimeout);

protected:
	Poco::URI m_uri;
	const Poco::Timespan m_httpTimeout;
};

}
