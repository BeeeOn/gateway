#ifndef GATEWAY_BELKIN_WEMO_SWITCH_H
#define GATEWAY_BELKIN_WEMO_SWITCH_H

#include <list>
#include <string>

#include <Poco/Net/SocketAddress.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/URI.h>

#include "belkin/BelkinWemoDevice.h"
#include "net/MACAddress.h"
#include "model/ModuleType.h"
#include "model/SensorData.h"

namespace BeeeOn {

/**
 * @brief The class represents Belkin WeMo Switch F7C027fr.
 * Provides functions to control the switch. It means turn on, turn off, get state.
 */
class BelkinWemoSwitch : public BelkinWemoDevice{
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
	bool requestModifyState(const ModuleID& moduleID, const double value) override;

	/**
	 * @brief Prepares SOAP message containing request state
	 * command and sends it to device via HTTP. If the device is
	 * not on network throws Poco::TimeoutException also in this
	 * case it is blocking. Request current state of the device
	 * and return it as SensorData.
	 * @return SensorData.
	 */
	SensorData requestState() override;

	Poco::Net::SocketAddress address() const;
	void setAddress(const Poco::Net::SocketAddress& address);
	std::list<ModuleType> moduleTypes() const override;
	std::string name() const override;

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

private:
	Poco::URI m_uri;
	Poco::Timespan m_httpTimeout;
};

}

#endif
