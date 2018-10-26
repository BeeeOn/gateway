#pragma once

#include <glib.h>
#include <gio/gio.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

#include <Poco/Condition.h>
#include <Poco/Mutex.h>
#include <Poco/RunnableAdapter.h>
#include <Poco/SharedPtr.h>
#include <Poco/Thread.h>
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
#include "util/WaitCondition.h"

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
	class Device;
	friend class DBusHciConnection;

	typedef Poco::SharedPtr<DBusHciInterface> Ptr;
	typedef std::function<bool(const std::string& path)> PathFilter;
	typedef std::pair<Poco::FastMutex, std::map<MACAddress, Device>> ThreadSafeDevices;

	/**
	 * @brief The class represents the Bluetooth Low Energy device and
	 * stores necessary data about device such as instance of device,
	 * handle of signal and timestamp of last rssi update. Also the class
	 * allows to store necessary data (signal handle, pointer to callback)
	 * when the device is watched.
	 */
	class Device {
	public:
		/**
		 * @param rssiHandle handle of signal on which is connected
		 * onDeviceRSSIChanged callback.
		 */
		Device(
			const GlibPtr<OrgBluezDevice1> device,
			const uint64_t rssiHandle);

		GlibPtr<OrgBluezDevice1> device() const
		{
			return m_device;
		}

		uint64_t rssiHandle() const
		{
			return m_rssiHandle;
		}

		void updateLastSeen()
		{
			m_lastSeen = Poco::Timestamp();
		}

		Poco::Timestamp lastSeen() const
		{
			return m_lastSeen;
		}

		uint64_t watchHandle() const
		{
			return m_watchHandle;
		}

		void watch(
			uint64_t watchHandle,
			Poco::SharedPtr<WatchCallback> callBack)
		{
			m_watchHandle = watchHandle;
			m_callBack = callBack;
		}

		void unwatch()
		{
			m_watchHandle = 0;
			m_callBack = nullptr;
		}

		bool isWatched() const
		{
			return !m_callBack.isNull();
		}

		std::string name();
		MACAddress macAddress();
		int16_t rssi();

	private:
		GlibPtr<OrgBluezDevice1> m_device;
		Poco::Timestamp m_lastSeen;
		uint64_t m_rssiHandle;
		uint64_t m_watchHandle;
		Poco::SharedPtr<WatchCallback> m_callBack;
	};


	/**
	 * @param name name of hci
	 * @param leMaxAgeRssi maximum time from the rssi update of the
	 * device to declare the device is available
	 * @param leMaxUnavailabilityTime maximum LE device unavailability
	 * time for device to be deleted
	 */
	DBusHciInterface(
		const std::string& name,
		const Poco::Timespan& leMaxAgeRssi,
		const Poco::Timespan& leMaxUnavailabilityTime);
	~DBusHciInterface();

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
	 * devices. The method returns devices whitch updated their RSSI
	 * before the given time.
	 */
	std::map<MACAddress, std::string> lescan(
		const Poco::Timespan &timeout) const override;

	/**
	 * @brief Uses BluezHciInterface to retrieve hci info.
	 */
	HciInfo info() const override;

	HciConnection::Ptr connect(
		const MACAddress& address,
		const Poco::Timespan& timeout) const override;

	void watch(
		const MACAddress& address,
		Poco::SharedPtr<WatchCallback> callBack) override;
	void unwatch(const MACAddress& address) override;

protected:
	/**
	 * @brief Callback handling the timeout event which stops
	 * the given GMainLoop.
	 */
	static gboolean onStopLoop(gpointer loop);

	/**
	 * @brief Callback handling the event of creating new device.
	 * The method adds the new device to m_devices and register
	 * onDeviceRSSIChanged callback.
	 */
	static void onDBusObjectAdded(
		GDBusObjectManager* objectManager,
		GDBusObject* object,
		gpointer userData);

	/**
	 * @brief Callback handling the event of RSSI changed. It is used
	 * to detecting that the device is available.
	 */
	static gboolean onDeviceRSSIChanged(
		OrgBluezDevice1* device,
		GVariant* properties,
		const gchar* const* invalidatedProperties,
		gpointer userData);

	/**
	 * @brief Callback handling the event of receiving advertising data.
	 */
	static gboolean onDeviceManufacturerDataRecieved(
		OrgBluezDevice1* device,
		GVariant* properties,
		const gchar* const* invalidatedProperties,
		gpointer userData);

	/**
	 * @brief Process the single advertising data and call callback
	 * stored in userData.
	 */
	static void processManufacturerData(
		OrgBluezDevice1* device,
		GVariant* value,
		gpointer userData);

private:
	/**
	 * @brief Checks periodically status of hci for 1 second. If the status of hci
	 * changes to required value the method ends.
	 * @throws TimeoutException in case of the hci is not in required status
	 * after the timeout expired.
	 */
	void waitUntilPoweredChange(GlibPtr<OrgBluezAdapter1> adapter, const bool powered) const;

	/**
	 * @brief Switch on the discovery on the adapter.
	 * @param trasnport type of device to be searched for (auto, bredr, le)
	 * @throws IOException in case of failure
	 */
	void startDiscovery(
		GlibPtr<OrgBluezAdapter1> adapter,
		const std::string& trasport) const;

	/**
	 * @brief Switch off the discovery on the adapter
	 */
	void stopDiscovery(GlibPtr<OrgBluezAdapter1> adapter) const;

	/**
	 * @brief Sets discovery to search given type of bluetooth devices.
	 * Possible types of devices are auto, bredr (classic) and le.
	 * @throws IOException in case of a failure
	 */
	void initDiscoveryFilter(
		GlibPtr<OrgBluezAdapter1> adapter,
		const std::string& trasport) const;

	/**
	 * @brief Obtains all known devices for Bluez deamon and returns them.
	 */
	std::vector<GlibPtr<OrgBluezDevice1>> processKnownDevices(
		GlibPtr<GDBusObjectManager> objectManager) const;

	/**
	 * @brief Removes the unavailable devices for specific time except watched devices.
	 */
	void removeUnvailableDevices() const;

	/**
	 * @brief The purpose of the method is to run GMainLoop that handles
	 * asynchronous events such as add new device during lescan() in separated thread.
	 */
	void runLoop();

	/**
	 * @brief Retrieves and returns all object paths from the given DBus object manager
	 * that match the pathFilter and the objectFilter.
	 */
	static std::vector<std::string> retrievePathsOfBluezObjects(
		GlibPtr<GDBusObjectManager> objectManager,
		PathFilter pathFilter,
		const std::string& objectFilter);

	/**
	 * @brief Creates object path for the hci stored in m_name attribute.
	 */
	static const std::string createAdapterPath(const std::string& name);

	/**
	 * @brief Creates object path for the device defined by it's MAC address.
	 */
	static const std::string createDevicePath(
		const std::string& name,
		const MACAddress& address);

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
	mutable WaitCondition m_resetCondition;

	GlibPtr<GMainLoop> m_loop;
	Poco::RunnableAdapter<DBusHciInterface> m_loopThread;
	Poco::Thread m_thread;
	GlibPtr<GDBusObjectManager> m_objectManager;
	uint64_t m_objectManagerHandle;
	mutable ThreadSafeDevices m_devices;
	mutable GlibPtr<OrgBluezAdapter1> m_adapter;
	Poco::Timespan m_leMaxAgeRssi;
	Poco::Timespan m_leMaxUnavailabilityTime;

	mutable Poco::Condition m_condition;
	mutable Poco::FastMutex m_statusMutex;
	mutable Poco::FastMutex m_discoveringMutex;
};

class DBusHciInterfaceManager : public HciInterfaceManager {
public:
	DBusHciInterfaceManager();

	void setLeMaxAgeRssi(const Poco::Timespan& time);
	void setLeMaxUnavailabilityTime(const Poco::Timespan& time);

	HciInterface::Ptr lookup(const std::string &name) override;

private:
	Poco::FastMutex m_mutex;
	std::map<std::string, DBusHciInterface::Ptr> m_interfaces;
	Poco::Timespan m_leMaxAgeRssi;
	Poco::Timespan m_leMaxUnavailabilityTime;
};

}
