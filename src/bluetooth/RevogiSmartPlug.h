#pragma once

#include <Poco/SharedPtr.h>
#include <Poco/UUID.h>

#include "bluetooth/RevogiDevice.h"

namespace BeeeOn {

/**
 * @brief The class represents Revogi Smart Meter Plug. It allows
 * to control all its modules.
 */
class RevogiSmartPlug : public RevogiDevice {
public:
	typedef Poco::SharedPtr<RevogiSmartPlug> Ptr;

	static const std::string PLUG_NAME;

public:
	RevogiSmartPlug(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const RefreshTime& refresh,
		const HciInterface::Ptr hci);
	~RevogiSmartPlug();

	void requestModifyState(
		const ModuleID& moduleID,
		const double value) override;

protected:
	/**
	 * <pre>
	 * | 4 B | on/off (1 B) | 3 B | power (2 B) | voltage (1 B) | current (2 B) | frequency (1 B) | 5 B |
	 * </pre>
	 */
	SensorData parseValues(const std::vector<unsigned char>& values) const override;

	void prependHeader(std::vector<unsigned char>& payload) const override;
};

}
