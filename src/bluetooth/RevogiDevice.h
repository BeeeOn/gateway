#pragma once

#include <list>
#include <string>
#include <vector>

#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/UUID.h>

#include "bluetooth/BLESmartDevice.h"
#include "bluetooth/HciConnection.h"
#include "bluetooth/HciInterface.h"
#include "model/ModuleType.h"
#include "net/MACAddress.h"

namespace BeeeOn {

/**
 * @brief Abstract class for Revogi devices.
 *
 * The class provides static method to create instance of specific
 * Reovig device class. Device identification is according to device
 * name located in the vendor specific characteristic. The class also
 * allows to obtain actual setting of the device and send commands.
 * The command consist of three parts: the prefix, the message body
 * and the suffix. In the suffix is located checksum.
 */
class RevogiDevice : public BLESmartDevice {
public:
	typedef Poco::SharedPtr<RevogiDevice> Ptr;

protected:
	/**
	 * UUID of characteristics containing actual values of all modules.
	 */
	static const Poco::UUID ACTUAL_VALUES_GATT;
	/**
	 * UUID of characteristics to modify device status.
	 */
	static const Poco::UUID WRITE_VALUES_GATT;
	/**
	 * UUID of characteristics containing device name.
	 */
	static const Poco::UUID UUID_DEVICE_NAME;
	static const std::string VENDOR_NAME;
	/**
	 * Writing this data to WRITE_VALUES_GATT characteristic triggers
	 * sending actual setting of the device to ACTUAL_VALUES_GATT
	 * characteristic.
	 */
	static const std::vector<unsigned char> NOTIFY_DATA;

public:
	RevogiDevice(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const std::string& productName,
		const std::list<ModuleType>& moduleTypes,
		const HciInterface::Ptr hci);

	~RevogiDevice();

	std::list<ModuleType> moduleTypes() const override;
	std::string vendor() const override;
	std::string productName() const override;

	/**
	 * @brief Retrieve the actual setting of the device and transform
	 * it to SensorData by particular implementation of the method
	 * parseValues().
	 */
	SensorData requestState() override;

	/**
	 * @brief The method returns true if the model ID of the device
	 * may be model ID of some Revogi device.
	 *
	 * Guyes have been creative about this value, not very helpful...
	 */
	static bool match(const std::string& modelID);

	/**
	 * @brief Creates Revogi device according to device name located
	 * in the vendor specific characteristic of the BLE device.
	 */
	static RevogiDevice::Ptr createDevice(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const HciInterface::Ptr hci,
		HciConnection::Ptr conn);

protected:
	/**
	 * @brief Convert actual setting of the device to SensorData.
	 */
	virtual SensorData parseValues(const std::vector<unsigned char>& values) const = 0;

	/**
	 * @brief Prepends header and appends footer to payload and then
	 * sends it to device.
	 */
	void sendWriteRequest(
		HciConnection::Ptr conn,
		std::vector<unsigned char> payload,
		const unsigned char checksum);

	virtual void prependHeader(std::vector<unsigned char>& payload) const = 0;
	virtual void appendFooter(
		std::vector<unsigned char>& payload,
		const unsigned char checksum) const;

private:
	std::string m_productName;
	std::list<ModuleType> m_moduleTypes;
};

}
