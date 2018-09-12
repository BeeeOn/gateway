#pragma once

#include <list>
#include <string>

#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/URI.h>
#include <Poco/Net/SocketAddress.h>

#include "model/SensorData.h"
#include "net/MACAddress.h"

namespace BeeeOn {

/**
 * @brief The class represents Belkin WeMo Link.
 * Provides functions to control the bulbs. It means turn on, turn off,
 * modify dim, get state of bulb.
 */
class BelkinWemoLink {
public:
	friend class BelkinWemoBulb;

	typedef Poco::SharedPtr<BelkinWemoLink> Ptr;
	typedef uint64_t BulbID;

private:
	BelkinWemoLink(const Poco::Net::SocketAddress& address);

public:
	/**
	 * @brief Creates belkin wemo link. If the device do not respond in
	 * specified timeout, Poco::TimeoutException is thrown.
	 * @param &address IP address and port where the device is listening.
	 * @param &timeout HTTP timeout.
	 * @return Belkin WeMo Link.
	 */
	static BelkinWemoLink::Ptr buildDevice(const Poco::Net::SocketAddress& address,
		const Poco::Timespan& timeout);

	/**
	 * @brief Prepares SOAP message containing request device list
	 * command and sends it to device via HTTP. If the device do not
	 * respond in specified timeout, Poco::TimeoutException is thrown.
	 * @return List of bulbIDs.
	 */
	std::list<BelkinWemoLink::BulbID> requestDeviceList();

	/**
	 * @brief Prepares SOAP message containing request modify state
	 * of proper device command and sends it to device via HTTP.
	 * If the device is not on network throws Poco::TimeoutException
	 * also in this case it is blocking.
	 * @return If the request was successful or not.
	 */
	bool requestModifyState(const BelkinWemoLink::BulbID bulbID,
		const int capability, const std::string& value);

	/**
	 * @brief Prepares SOAP message containing request state
	 * of proper device command and sends it to device via HTTP.
	 * If the device do not respond in specified timeout,
	 * Poco::TimeoutException is thrown. Return the body of HTTP response.
	 */
	std::string requestDeviceState(const BelkinWemoLink::BulbID bulbID);

	Poco::Net::SocketAddress address() const;
	void setAddress(const Poco::Net::SocketAddress& address);
	MACAddress macAddress() const;

	uint32_t countOfBulbs();
	Poco::FastMutex& lock();

	/**
	 * @brief It compares two links based on MAC address.
	 */
	bool operator==(const BelkinWemoLink& bwl) const;

private:
	void requestDeviceInfo();

	/**
	 * @brief Method is called by constructor of BelkinWemoBulb.
	 */
	void incrementCountOfBulbs();

	/**
	 * @brief Method is called by destructor of BelkinWemoBulb.
	 */
	void decrementCountOfBulbs();

private:
	Poco::Net::SocketAddress m_address;
	MACAddress m_macAddr;
	std::string m_udn;

	Poco::FastMutex m_lockCountOfBulbs;
	int m_countOfBulbs;

	Poco::FastMutex m_lock;
	Poco::Timespan m_httpTimeout;
};

}
