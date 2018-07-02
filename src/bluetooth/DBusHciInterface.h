#ifndef GATEWAY_DBUS_HCI_INTERFACE_H
#define GATEWAY_DBUS_HCI_INTERFACE_H

#include <glib.h>
#include <gio/gio.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

#include <Poco/Condition.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/Timestamp.h>

#include "bluetooth/HciInfo.h"
#include "bluetooth/HciInterface.h"
#include "bluetooth/GlibPtr.h"
#include "bluetooth/org-bluez-adapter1.h"
#include "bluetooth/org-bluez-device1.h"
#include "bluetooth/org-bluez-gattcharacteristic1.h"
#include "net/MACAddress.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief The class realizes communication with BlueZ deamon using D-Bus.
 * It allows to find new BLE and Bluetooth Classic devices and send read/write requests.
 *
 * The class uses BluezHciInterface to work with Bluetooth Classic devices and
 * to retrieve hci info. It requires a running instance of the D-Bus service "org.bluez"
 * to work wth BLE devices. Firstly it is needed to find BLE devices in a bluetooth
 * network to create D-Bus device objects. Accessing the BLE devices is possible via
 * D-Bus device objects (defined by path "/org/bluez/hci0/dev_FF_FF_FF_FF_FF_FF").
 * The D-Bus device objects provide methods to perform actions as connect, read,
 * write and disconnect.
 */
class DBusHciInterface : public HciInterface, Loggable {
public:
	typedef std::function<bool(const std::string& path)> PathFilter;

	/**
	 * @param name name of hci
	 * @param statusMutex lock ensures exclusive access to state of hci
	 */
	DBusHciInterface(
		const std::string& name,
		Poco::SharedPtr<Poco::FastMutex> statusMutex);

	/**
	 * @brief Sets hci interface down.
	 * @throws IOException in case of a failure
	 */
	void down() const;

	void up() const override;
	void reset() const override;

	/**
	 * @brief Uses detect method of BluezHciInterface class to
	 * check state of device with MACAddress.
	 */
	bool detect(const MACAddress &address) const override;

	/**
	 * @brief Uses scan method of BluezHciInterface class to discover
	 * the bluetooh classic devices.
	 */
	std::map<MACAddress, std::string> scan() const override;

	/**
	 * @brief Scans the Bluetooth LE network and returns all available
	 * devices. The method starts with the obtaining of all known devices
	 * to Bluez deamon. Then the bluetooth adapter starts discovery of new
	 * devices whitch extends collection of devices. Availability of device
	 * is determined by paramter RSSI. If the RSSI parametr is 0 then the
	 * device is unavailable.
	 */
	std::map<MACAddress, std::string> lescan(
		const Poco::Timespan &timeout) const override;

	/**
	 * @brief Uses BluezHciInterface to retrieve hci info.
	 */
	HciInfo info() const override;

protected:
	static gboolean onStopLoop(gpointer loop);
	static void onDBusObjectAdded(
		GDBusObjectManager* objectManager,
		GDBusObject* object,
		gpointer userData);

private:
	/**
	 * @brief Checks periodically status of hci for 1 second. If the status of hci
	 * changes to required value the method ends.
	 * @throws TimeoutException in case of the hci is not in required status
	 * after the timeout expired.
	 */
	void waitUntilPoweredChange(const std::string& path, const bool powered) const;

	/**
	 * @brief Sets discovery to search given type of bluetooth devices.
	 * Possible types of devices are auto, bredr (classic) and le.
	 * @throws IOException in case of a failure
	 */
	void initDiscoveryFilter(
		GlibPtr<OrgBluezAdapter1> adapter,
		const std::string& trasport) const;

	/**
	 * @brief Obtains all known devices for Bluez deamon and stores them into
	 * the paramater devices.
	 */
	void processKnownDevices(
		GlibPtr<GDBusObjectManager> objectManager,
		std::map<MACAddress, std::string>& devices) const;

	/**
	 * @brief Retrieves and returns all object paths from the given DBus object manager
	 * that match the pathFilter and the objectFilter.
	 */
	std::vector<std::string> retrievePathsOfBluezObjects(
		GlibPtr<GDBusObjectManager> objectManager,
		PathFilter pathFilter,
		const std::string& objectFilter) const;

	/**
	 * @brief Creates object path for the hci stored in m_name attribute.
	 */
	const std::string createAdapterPath() const;

	/**
	 * @brief Creates object path for the device defined by it's MAC address.
	 */
	const std::string createDevicePath(const MACAddress& address) const;

	/**
	 * @brief Creates object manager for the Bluez DBus server that
	 * is defined by "org.bluez".
	 * @throws IOException in case of a failure
	 */
	static GlibPtr<GDBusObjectManager> createBluezObjectManager();

	/**
	 * @brief Returns DBus object of hci defined by it's path.
	 * @throws IOException in case of a failure
	 */
	static GlibPtr<OrgBluezAdapter1> retrieveBluezAdapter(const std::string& path);

	/**
	 * @brief Returns DBus object of device defined by it's path.
	 * @throws IOException in case of a failure
	 */
	static GlibPtr<OrgBluezDevice1> retrieveBluezDevice(const std::string& path);

private:
	std::string m_name;
	mutable Poco::Condition m_condition;
	mutable Poco::SharedPtr<Poco::FastMutex> m_statusMutex;
};

class DBusHciInterfaceManager : public HciInterfaceManager {
public:
	DBusHciInterfaceManager();

	HciInterface::Ptr lookup(const std::string &name) override;

private:
	Poco::SharedPtr<Poco::FastMutex> m_statusMutex;
};

}

#endif
