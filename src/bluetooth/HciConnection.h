#pragma once

#include <vector>

#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/UUID.h>

#include "net/MACAddress.h"

namespace BeeeOn {

/**
 * @brief The interface class represents connection with BLE device.
 */
class HciConnection {
public:
	typedef Poco::SharedPtr<HciConnection> Ptr;

	virtual ~HciConnection();

	/**
	 * @brief Sends read request to device defined by MAC address. The read
	 * characteristic is defined by UUID of characteristic.
	 * @throws IOException in case of a failure
	 * @throws NotFoundException when the characteristic not found
	 * @throws TimeoutException when the services not resolved
	 */
	virtual std::vector<unsigned char> read(const Poco::UUID& uuid) = 0;

	/**
	 * @brief Sends write request to device defined by MAC address. The modified
	 * characteristic is defined by UUID of characteristic.
	 * @throws IOException in case of a failure
	 * @throws NotFoundException when the characteristic not found
	 * @throws TimeoutException when the services not resolved
	 */
	virtual void write(
		const Poco::UUID& uuid,
		const std::vector<unsigned char>& value) = 0;

	/**
	 * @brief The method returns data from the given notifying characteristic
	 * to which the device sends data based on sending the message to
	 * another given characteristic.
	 * @throws IOException in case of a failure
	 * @throws NotFoundException when the characteristic not found
	 * @throws TimeoutException when the data is not recieved within
	 * the certain timeout
	 */
	virtual std::vector<unsigned char> notifiedWrite(
		const Poco::UUID& notifyUuid,
		const Poco::UUID& writeUuid,
		const std::vector<unsigned char>& value,
		const Poco::Timespan& notifyTimeout) = 0;
};

}
