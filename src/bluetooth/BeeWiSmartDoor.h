#pragma once

#include <Poco/SharedPtr.h>

#include "bluetooth/BeeWiDevice.h"
#include "bluetooth/HciConnection.h"

namespace BeeeOn {

/**
 * @brief The class represents BeeWi door sensor. It allows
 * to parse recieved data from the device. Its modules are
 * open/close and battery level.
 */
class BeeWiSmartDoor : public BeeWiDevice {
public:
	typedef Poco::SharedPtr<BeeWiSmartDoor> Ptr;

	static const std::string NAME;

protected:
	/**
	 * The intention of this constructor is only for testing.
	 */
	BeeWiSmartDoor(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const RefreshTime& refresh,
		const HciInterface::Ptr hci);

public:
	BeeWiSmartDoor(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const RefreshTime& refresh,
		const HciInterface::Ptr hci,
		HciConnection::Ptr conn);
	~BeeWiSmartDoor();

	/**
	 * <pre>
	 * | ID (1 B) | 1 B | open/close (1 B) | 1 B | battery (1 B) |
	 * </pre>
	 */
	SensorData parseAdvertisingData(
		const std::vector<unsigned char>& data) const override;

	static bool match(const std::string& modelID);
};

}
