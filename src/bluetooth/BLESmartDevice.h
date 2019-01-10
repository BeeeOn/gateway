#pragma once

#include <list>
#include <string>
#include <vector>

#include <Poco/SharedPtr.h>
#include <Poco/SynchronizedObject.h>
#include <Poco/Timespan.h>

#include "bluetooth/HciInterface.h"
#include "model/DeviceID.h"
#include "model/ModuleID.h"
#include "model/ModuleType.h"
#include "model/SensorData.h"
#include "net/MACAddress.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief Abstract class representing generic Bluetooth Low Energy smart device.
 */
class BLESmartDevice :
	protected Poco::SynchronizedObject,
	protected Loggable {
public:
	typedef Poco::SharedPtr<BLESmartDevice> Ptr;

	/**
	 * @param &address MAC address of the device
	 * @param &timeout timeout of actions as connect, read, write
	 * @param hci reference to HCI
	 */
	BLESmartDevice(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const HciInterface::Ptr hci);
	virtual ~BLESmartDevice();

	DeviceID deviceID() const;
	MACAddress macAddress() const;

	virtual std::list<ModuleType> moduleTypes() const = 0;
	virtual std::string productName() const = 0;
	virtual std::string vendor() const = 0;

	/**
	 * @brief When the device supports processing of advertising data,
	 * it should call a watch() on the given HciInterface in this method
	 * and a unwatch() in the destructor. When the method is re-called
	 * it do not have any effect.
	 */
	virtual void pair(
		Poco::SharedPtr<HciInterface::WatchCallback> callback);

	/**
	 * @brief Modifies the device module given by moduleID to a given
	 * value.
	 * @throws IOException in case of communication failure.
	 * @throws NotImplementedException if the device does not support
	 * modification of its state.
	 */
	virtual void requestModifyState(
		const ModuleID& moduleID,
		const double value);

	/**
	 * @brief Obtains the actual state of the device.
	 * @throws IOException in case of communication failure.
	 * @throws ProtocolException in case of bad received message.
	 * @throws NotImplementedException if the device does not support
	 * obtaining of its state.
	 */
	virtual SensorData requestState();

	/**
	 * @brief Transforms advertising data to SensorData.
	 * @throws ProtocolException in case of bad received advertising data.
	 * @throws NotImplementedException if the device does not support
	 * processing of its advertising data.
	 */
	virtual SensorData parseAdvertisingData(
		const std::vector<unsigned char>& data) const;

protected:
	DeviceID m_deviceId;
	MACAddress m_address;
	Poco::Timespan m_timeout;

	HciInterface::Ptr m_hci;
};

}
