#pragma once

#include <Poco/SharedPtr.h>
#include <Poco/UUID.h>

#include "bluetooth/BeeWiDevice.h"

namespace BeeeOn {

/**
 * @brief The class represents BeeWi smart led light. It allows
 * to gather and to control all its modules.
 */
class BeeWiSmartLite : public BeeWiDevice {
public:
	typedef Poco::SharedPtr<BeeWiSmartLite> Ptr;

	static const std::string NAME;

private:
	/**
	 * UUID of characteristics containing actual values of all sensor modules.
	 */
	static const Poco::UUID ACTUAL_VALUES;
	/**
	 * UUID of characteristics to modify device status.
	 */
	static const Poco::UUID WRITE_VALUES;

public:
	enum Command {
		ON_OFF = 0x10,
		COLOR_TEMPERATURE = 0x11,
		BRIGHTNESS = 0x12,
		COLOR = 0x13
	};

	BeeWiSmartLite(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const RefreshTime& refresh,
		const HciInterface::Ptr hci);
	~BeeWiSmartLite();

	void requestModifyState(
		const ModuleID& moduleID,
		const double value) override;

	/**
	* <pre>
	 * | ID (1 B) | 1 B | on/off (1 B) | 1 B | brightness (4 b) | color temperature (4 b) | color (3 B) |
	 * </pre>
	 */
	SensorData parseAdvertisingData(
		const std::vector<unsigned char>& data) const override;

	static bool match(const std::string& modelID);

protected:
	unsigned int brightnessToPercentages(const double value) const;
	unsigned char brightnessFromPercentages(const double percents) const;

	unsigned int colorTempToKelvins(const double value) const;
	unsigned char colorTempFromKelvins(const double temperature) const;
};

}
