#pragma once

#include <list>
#include <string>

#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/Dynamic/Var.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/SocketAddress.h>

#include "credentials/PasswordCredentials.h"
#include "net/HTTPEntireResponse.h"
#include "net/MACAddress.h"
#include "philips/PhilipsHueBridgeInfo.h"
#include "util/CryptoConfig.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief The class represents Philips Hue Bridge.
 * Provides functions to control the bulbs. It means turn on, turn off,
 * modify dim, get state of bulb.
 */
class PhilipsHueBridge : protected Loggable {
public:
	friend class PhilipsHueBulb;

	typedef Poco::SharedPtr<PhilipsHueBridge> Ptr;
	typedef uint64_t BulbID;

	static const Poco::Timespan SLEEP_BETWEEN_ATTEMPTS;

private:
	PhilipsHueBridge(const Poco::Net::SocketAddress& address);

public:
	/**
	 * @brief Creates Philips Hue Bridge. If the device do not respond in
	 * specified timeout, Poco::TimeoutException is thrown.
	 * @param &address IP address and port where the device is listening.
	 * @param &timeout HTTP timeout.
	 * @return Philips Hue Bridge.
	 */
	static PhilipsHueBridge::Ptr buildDevice(const Poco::Net::SocketAddress& address,
		const Poco::Timespan& timeout);

	/**
	 * @brief Authorization of the gateway to Philips Hue Bridge. It starts
	 * by sending an HTTP authorization request. After that, the user has to press
	 * the button on the bridge at a certain time. This generates a username by
	 * which we can control the bridge.
	 */
	std::string authorize(const std::string& deviceType = "BeeeOn#gateway");

	/**
	 * @brief Prepares POST HTTP request containing request to search new devices
	 * command and sends it to device via HTTP. If the device do not
	 * respond in specified timeout, Poco::TimeoutException is thrown.
	 */
	void requestSearchNewDevices();

	/**
	 * @brief Prepares GET HTTP request containing request device list
	 * command and sends it to device via HTTP. If the device do not
	 * respond in specified timeout, Poco::TimeoutException is thrown.
	 * @return List of bulbs. Each bulb contains a type, ordinal number and identifier.
	 * Example: {"Dimmable light", {1, 8877665544332211}}
	 */
	std::list<std::pair<std::string, std::pair<uint32_t, PhilipsHueBridge::BulbID>>> requestDeviceList();

	/**
	 * @brief Prepares JSON message containing request modify state
	 * command of proper device and sends it to device via HTTP.
	 * If the device do not respond in specified timeout,
	 * Poco::TimeoutException is thrown.
	 * @return If the request was successful or not.
	 */
	bool requestModifyState(const uint32_t ordinalNumber,
		const std::string& capability, const Poco::Dynamic::Var value);

	/**
	 * @brief Prepares GET HTTP request containing request state
	 * of proper device command and sends it to device via HTTP.
	 * If the device do not respond in specified timeout,
	 * Poco::TimeoutException is thrown. Return the body of HTTP response.
	 */
	std::string requestDeviceState(const uint32_t ordinalNumber);

	Poco::Net::SocketAddress address() const;
	void setAddress(const Poco::Net::SocketAddress& address);
	MACAddress macAddress() const;
	std::string username() const;
	void setCredentials(const Poco::SharedPtr<PasswordCredentials> credential,
		const Poco::SharedPtr<CryptoConfig> config);

	uint32_t countOfBulbs();
	Poco::FastMutex& lock();

	PhilipsHueBridgeInfo info();

private:
	void requestDeviceInfo();

	/**
	 * @brief Decodes BulbID from a string which is in format MAC address - endpoind id
	 * (AA:BB:CC:DD:EE:FF:00:11-XX).
	 */
	PhilipsHueBridge::BulbID decodeBulbID(const std::string& strBulbID) const;

	/**
	 * @brief Method is called by constructor of PhilipsBulb.
	 */
	void incrementCountOfBulbs();

	/**
	 * @brief Method is called by destructor of PhilipsBulb.
	 */
	void decrementCountOfBulbs();

	HTTPEntireResponse sendRequest(Poco::Net::HTTPRequest& request, const std::string& message,
		const Poco::Net::SocketAddress& address, const Poco::Timespan& timeout);

private:
	Poco::Net::SocketAddress m_address;
	MACAddress m_macAddr;
	Poco::SharedPtr<PasswordCredentials> m_credential;
	Poco::SharedPtr<CryptoConfig> m_cryptoConfig;

	Poco::FastMutex m_lockCountOfBulbs;
	int m_countOfBulbs;

	Poco::FastMutex m_lock;
	Poco::Timespan m_httpTimeout;
};

}
