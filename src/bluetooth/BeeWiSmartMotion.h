#pragma once

#include <Poco/SharedPtr.h>

#include "bluetooth/BeeWiDevice.h"
#include "bluetooth/HciConnection.h"

namespace BeeeOn {

/**
 * @brief The class represents BeeWi motion sensor. It allows
 * to parse recieved data from the device. Its modules are
 * motion and battery level.
 */
class BeeWiSmartMotion : public BeeWiDevice {
public:
	typedef Poco::SharedPtr<BeeWiSmartMotion> Ptr;

	static const std::string NAME;

protected:
	/**
	 * The intention of this constructor is only for testing.
	 */
	BeeWiSmartMotion(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const RefreshTime& refresh,
		const HciInterface::Ptr hci);

public:
	BeeWiSmartMotion(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const RefreshTime& refresh,
		const HciInterface::Ptr hci,
		HciConnection::Ptr conn);
	~BeeWiSmartMotion();

	/**
	 * <pre>
	 * | ID (1 B) | 1 B | motion (1 B) | 1 B | battery (1 B) |
	 * </pre>
	 */
	SensorData parseAdvertisingData(
		const std::vector<unsigned char>& data) const override;

	static bool match(const std::string& modelID);
};

}
