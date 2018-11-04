#pragma once

#include <string>
#include <vector>

#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/UUID.h>

#include "bluetooth/GlibPtr.h"
#include "bluetooth/HciConnection.h"
#include "bluetooth/org-bluez-device1.h"
#include "bluetooth/org-bluez-gattcharacteristic1.h"
#include "net/MACAddress.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief The class represents connection with Bluetooth Low energy device.
 * It allows sending read/write requests.
 */
class DBusHciConnection:
	public HciConnection,
	Loggable {
public:
	typedef Poco::SharedPtr<DBusHciConnection> Ptr;

	DBusHciConnection(
		const std::string& hciName,
		GlibPtr<OrgBluezDevice1> device,
		const Poco::Timespan& timeout);
	~DBusHciConnection();

	/**
	 * @brief Reads value from the GATT characteristic defined
	 * by UUID.
	 */
	std::vector<unsigned char> read(const Poco::UUID& uuid) override;

	/**
	 * @brief Writes value to the GATT characteristic defined
	 * by UUID.
	 */
	void write(
		const Poco::UUID& uuid,
		const std::vector<unsigned char>& value) override;

	/**
	 * @brief The method starts notifying the characteristic
	 * given by notifyUuid. The callback is connected to signal
	 * property changed of the notifying characteristic. Then
	 * the given data are sent to the characteristic defined by
	 * writeUuid. After that it waits for a maximum of notifyTimeout
	 * for the data to be recieved.
	 */
	std::vector<unsigned char> notifiedWrite(
		const Poco::UUID& notifyUuid,
		const Poco::UUID& writeUuid,
		const std::vector<unsigned char>& value,
		const Poco::Timespan& notifyTimeout) override;

protected:
	static gboolean onDeviceServicesResolved(
		OrgBluezDevice1* device,
		GVariant* properties,
		const gchar* const* invalidatedProperties,
		gpointer userData);

	static gboolean onCharacteristicValueChanged(
		OrgBluezGattCharacteristic1*,
		GVariant* properties,
		const gchar* const*,
		gpointer userData);

private:
	/**
	 * @brief Disconnects device.
	 */
	void disconnect();

	/**
	 * @brief Resolves the services of device.
	 * @throws TimeoutException in case of a failure
	 */
	void resolveServices();

	/**
	 * @brief Writes value to the GATT characteristic defined
	 * by UUID.
	 */
	void doWrite(
		const Poco::UUID& uuid,
		const std::vector<unsigned char>& value);

	/**
	 * @brief Tries to find GATT characteristic defined by UUID for
	 * the given device. If the characteristic is not found it returns nullptr.
	 */
	GlibPtr<OrgBluezGattCharacteristic1> findGATTCharacteristic(
		const Poco::UUID& uuid);

	/**
	 * @brief Returns DBus object of GATT characteristic defined by it's path.
	 * @throws IOException in case of a failure
	 */
	static GlibPtr<OrgBluezGattCharacteristic1> retrieveBluezGATTCharacteristic(
		const std::string& path);

private:
	std::string m_hciName;
	GlibPtr<OrgBluezDevice1> m_device;
	MACAddress m_address;
	Poco::Timespan m_timeout;

	Poco::FastMutex m_writeMutex;
};

}
