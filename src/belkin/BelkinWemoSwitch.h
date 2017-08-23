#ifndef GATEWAY_BELKIN_WEMO_SWITCH_H
#define GATEWAY_BELKIN_WEMO_SWITCH_H

#include <list>
#include <string>

#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/Mutex.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/URI.h>

#include "net/MACAddress.h"
#include "model/ModuleType.h"
#include "model/DeviceID.h"
#include "model/SensorData.h"
#include "net/HTTPEntireResponse.h"

namespace BeeeOn {

/**
 * @brief The class represents Belkin WeMo Switch F7C027fr.
 * Provides functions to control the switch. It means turn on, turn off, get state.
 */
class BelkinWemoSwitch {
public:
	typedef Poco::SharedPtr<BelkinWemoSwitch> Ptr;

	BelkinWemoSwitch();
	~BelkinWemoSwitch();

	/**
	 * @brief Creates belkin wemo switch. If the device is not on network
	 * throws Poco::TimeoutException also in this case it is blocking.
	 * @param &address IP address and port where the device is listening.
	 * @param &timeout HTTP timeout.
	 * @return Belkin WeMo Switch.
	 */
	static BelkinWemoSwitch::Ptr buildDevice(const Poco::Net::SocketAddress& address,
		const Poco::Timespan& timeout);

	/**
	 * @brief It sets the switch to the given state.
	 */
	bool requestModifyState(const ModuleID& moduleID, const bool value);

	/**
	 * @brief Prepares SOAP message containing request state
	 * command and sends it to device via HTTP. If the device is
	 * not on network throws Poco::TimeoutException also in this
	 * case it is blocking. Request current state of the device
	 * and return it as SensorData.
	 * @return SensorData.
	 */
	SensorData requestState() const;

	DeviceID deviceID() const;
	Poco::Net::SocketAddress address() const;
	void setAddress(const Poco::Net::SocketAddress& address);
	std::list<ModuleType> moduleTypes() const;
	std::string name() const;
	Poco::FastMutex& lock();

	/**
	 * @brief It compares two switches based on DeviceID.
	 */
	bool operator==(const BelkinWemoSwitch& bws) const;

protected:
	BelkinWemoSwitch(const Poco::Net::SocketAddress& address);

	/**
	 * @brief Prepares SOAP message containing turn on command
	 * and sends it to device via HTTP. If the device is not on
	 * network throws Poco::TimeoutException also in this case
	 * it is blocking.
	 * @return If the request was successful or not.
	 */
	bool turnOn() const;

	/**
	 * @brief Prepares SOAP message containing turn off command
	 * and sends it to device via HTTP. If the device is not on
	 * network throws Poco::TimeoutException also in this case
	 * it is blocking.
	 * @return If the request was successful or not.
	 */
	bool turnOff() const;

	/**
	 * @brief Prepares SOAP message containing request Mac address
	 * command and sends it to device via HTTP. If the device is not
	 * on network throws Poco::TimeoutException also in this case
	 * it is blocking. Request mac address of the device.
	 * @return Mac address of the device.
	 */
	MACAddress requestMacAddr();

	/**
	 * @brief Called internally when constructing the instance.
	 * Creates DeviceID based on Mac address of device.
	 */
	void buildDeviceID();

	Poco::XML::Node* findNode(Poco::XML::NodeIterator& iterator, const std::string& name) const;
	HTTPEntireResponse sendHTTPRequest(Poco::Net::HTTPRequest& request, const std::string& msg,
		const Poco::Timespan& timeout) const;

private:
	DeviceID m_deviceId;
	Poco::URI m_uri;
	Poco::Timespan m_httpTimeout;
	Poco::FastMutex m_lock;
};

}

#endif
