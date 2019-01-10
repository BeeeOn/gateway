#pragma once

#include <Poco/SharedPtr.h>
#include <Poco/UUID.h>

#include "bluetooth/BeeWiDevice.h"
#include "bluetooth/HciConnection.h"

namespace BeeeOn {

/**
 * @brief The class represents BeeWi smart switch. It allows
 * to gather and to control all its modules.
 */
class BeeWiSmartWatt : public BeeWiDevice {
public:
	typedef Poco::SharedPtr<BeeWiSmartWatt> Ptr;

	static const std::string NAME;

private:
	/**
	 * UUID of characteristics containing actual values of all sensor modules.
	 */
	static const Poco::UUID ACTUAL_VALUES;
	/**
	 * UUID of characteristics to switch on/off the switch.
	 */
	static const Poco::UUID ON_OFF;
	/**
	 * UUID of characteristics to switch on/off the light on the switch.
	 */
	static const Poco::UUID LIGHT_ON_OFF;

protected:
	/**
	 * The intention of this constructor is only for testing.
	 */
	BeeWiSmartWatt(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const HciInterface::Ptr hci);

public:
	BeeWiSmartWatt(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const HciInterface::Ptr hci,
		HciConnection::Ptr conn);
	~BeeWiSmartWatt();

	void requestModifyState(
		const ModuleID& moduleID,
		const double value) override;
	SensorData requestState() override;

	/**
	 * <pre>
	 * | ID (1 B) | 1 B | on/off (1 B) | 3 B | power (2 B) | 5 B |
	 * </pre>
	 */
	SensorData parseAdvertisingData(
		const std::vector<unsigned char>& data) const override;

	static bool match(const std::string& modelID);

protected:
	/**
	* <pre>
	 * | on/off (1 B) | power (2 B) | voltage (1 B) | current (2 B) | frequency (1 B) |
	 * </pre>
	 */
	SensorData parseValues(std::vector<unsigned char>& values) const;
};

}
