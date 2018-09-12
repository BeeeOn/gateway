#pragma once

#include <list>
#include <string>

#include <Poco/Net/SocketAddress.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>

#include "belkin/BelkinWemoStandaloneDevice.h"
#include "model/ModuleType.h"
#include "model/SensorData.h"

namespace BeeeOn {

/**
 * @brief The class represents Belkin WeMo Switch F7C027fr.
 * Provides functions to control the switch. It means turn on, turn off, get state.
 */
class BelkinWemoSwitch : public BelkinWemoStandaloneDevice {
public:
	typedef Poco::SharedPtr<BelkinWemoSwitch> Ptr;

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

	std::list<ModuleType> moduleTypes() const override;
	std::string name() const override;

	/**
	 * @brief It compares two switches based on DeviceID.
	 */
	bool operator==(const BelkinWemoSwitch& bws) const;

protected:
	BelkinWemoSwitch(const Poco::Net::SocketAddress& address);
};

}
