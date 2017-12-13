#ifndef GATEWAY_BELKIN_WEMO_DIMMER_H
#define GATEWAY_BELKIN_WEMO_DIMMER_H

#include <list>
#include <string>

#include <Poco/Timespan.h>
#include <Poco/SharedPtr.h>
#include <Poco/Net/SocketAddress.h>

#include "belkin/BelkinWemoStandaloneDevice.h"
#include "model/ModuleType.h"
#include "model/SensorData.h"

namespace BeeeOn {

/**
 * @brief The class represents Belkin WeMo Dimmer F7C059.
 * Provides functions to control the switch. It means turn on, turn off,
 * modify brigtness, get state.
 */
class BelkinWemoDimmer : public BelkinWemoStandaloneDevice {
public:
	typedef Poco::SharedPtr<BelkinWemoDimmer> Ptr;

private:
	BelkinWemoDimmer(const Poco::Net::SocketAddress& address);

public:
	/**
	 * @brief Creates belkin wemo dimmer. If the device do not respond in
	 * specified timeout, Poco::TimeoutException is thrown.
	 * @param &address IP address and port where the device is listening.
	 * @param &timeout HTTP timeout.
	 */
	static BelkinWemoDimmer::Ptr buildDevice(const Poco::Net::SocketAddress& address,
		const Poco::Timespan& timeout);

	/**
	 * @brief Modifies the dimmer's given module to the given value.
	 */
	bool requestModifyState(const ModuleID& moduleID, const double value) override;

	/**
	 * @brief Prepares SOAP message containing request state
	 * command and sends it to device via HTTP. If the device
	 * do not respond in specified timeout, Poco::TimeoutException
	 * is thrown. Request current state of the device and return
	 * it as SensorData.
	 * @return SensorData.
	 */
	SensorData requestState() override;

	std::list<ModuleType> moduleTypes() const override;
	std::string name() const override;

	/**
	 * @brief It compares two switches based on DeviceID.
	 */
	bool operator==(const BelkinWemoDimmer& bw) const;
};

}

#endif
