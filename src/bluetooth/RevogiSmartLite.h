#pragma once

#include <Poco/SharedPtr.h>

#include "bluetooth/RevogiRGBLight.h"

namespace BeeeOn {

/**
 * @brief The class represents Revogi Delite-1748 bulb. It allows
 * to control all its modules.
 */
class RevogiSmartLite : public RevogiRGBLight {
public:
	typedef Poco::SharedPtr<RevogiSmartLite> Ptr;

	static const std::string LIGHT_NAME;

public:
	RevogiSmartLite(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const RefreshTime& refresh,
		const HciInterface::Ptr hci);
	~RevogiSmartLite();

	void requestModifyState(
		const ModuleID& moduleID,
		const double value) override;

protected:
	void modifyStatus(const double value, const HciConnection::Ptr conn);
	void modifyBrightness(
		const double value,
		const unsigned char colorTemperature,
		const HciConnection::Ptr conn);
	void modifyColorTemperature(const double value, const HciConnection::Ptr conn);

	unsigned char colorTempFromKelvins(const double temperature) const;
	unsigned int colorTempToKelvins(const double value) const;

	/**
	 * <pre>
	 * | 4 B | rgb (3 B) | brightness/on_off (1 B) | color temperature (1 B) | rgb mode (1 B) | 8 B |
	 * </pre>
	 */
	SensorData parseValues(const std::vector<unsigned char>& values) const override;
};

}
