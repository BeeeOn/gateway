#pragma once

#include <Poco/SharedPtr.h>

#include "bluetooth/RevogiRGBLight.h"

namespace BeeeOn {

/**
 * @brief The class represents Revogi Delite-ED33 smart candle. It allows
 * to control all its modules.
 */
class RevogiSmartCandle : public RevogiRGBLight {
public:
	typedef Poco::SharedPtr<RevogiSmartCandle> Ptr;

	static const std::string LIGHT_NAME;

public:
	RevogiSmartCandle(const MACAddress& address, const Poco::Timespan& timeout);
	~RevogiSmartCandle();

	void requestModifyState(
		const ModuleID& moduleID,
		const double value,
		const HciInterface::Ptr hci) override;

protected:
	/**
	 * <pre>
	 * | 4 B | rgb (3 B) | brightness/on_off (1 B) | 10 B |
	 * </pre>
	 */
	SensorData parseValues(const std::vector<unsigned char>& values) const override;
};

}
