#pragma once

#include <list>
#include <map>
#include <string>

#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <Poco/RunnableAdapter.h>
#include <Poco/Thread.h>
#include <Poco/Timespan.h>

#include "bluetooth/BluetoothDevice.h"
#include "bluetooth/HciInterface.h"
#include "commands/DeviceAcceptCommand.h"
#include "core/DongleDeviceManager.h"
#include "model/DeviceID.h"
#include "model/SensorData.h"

namespace BeeeOn {

class BluetoothAvailabilityManager : public DongleDeviceManager {
public:
	BluetoothAvailabilityManager();

	/**
	 * Main thread.
	 * @throw when is any problem with scan
	 */
	void dongleAvailable() override;

	bool dongleMissing() override;

	void dongleFailed(const FailDetector &dongleStatus) override;

	/**
	 * @brief Recognizes compatible dongle by testing HotplugEvent property
	 * as <code>bluetooth.BEEEON_DONGLE == bluetooth</code>.
	 */
	std::string dongleMatch(const HotplugEvent &e) override;

	void stop() override;

	Poco::Timespan detectAll(const HciInterface &hci);

	/**
	 * How often will be scan of paired devices
	 */
	void setWakeUpTime(const Poco::Timespan &time);

	void setModes(const std::list<std::string> &modes);

	/**
	 * Set time for a single LE scan when testing device availability.
	 */
	void setLEScanTime(const Poco::Timespan &time);

	/**
	 * Set HciInterfaceManager implementation.
	 */
	void setHciManager(HciInterfaceManager::Ptr manager);

	void handleRemoteStatus(
		const DevicePrefix &prefix,
		const std::set<DeviceID> &devices,
		const DeviceStatusHandler::DeviceValues &values) override;

protected:
	void handleAccept(const DeviceAcceptCommand::Ptr cmd) override;
	AsyncWork<>::Ptr startDiscovery(const Poco::Timespan &timeout) override;
	AsyncWork<std::set<DeviceID>>::Ptr startUnpair(
			const DeviceID &id,
			const Poco::Timespan &timeout) override;
	void notifyDongleRemoved() override;

private:
	/*
	 * Scan for all paired devices. Any inactive device is
	 * temporarily saved. Information about active devices
	 * is immediatelly shipped.
	 * @return list of (potentially) inactive devices
	 */
	std::list<DeviceID> detectClassic(const HciInterface &hci);

	/*
	 * Scan for BLE devices.
	 * There is no inactive list.
	 * The last BLE scan result is stored in m_leScanCache.
	 * m_leScanCache exists for fast result of listen command.
	 */
	void detectLE(const HciInterface &hci);

	bool haveTimeForInactive(Poco::Timespan elapsedTime);

	/**
	 * @brief Creates BluetoothDevice for every device known
	 * to DeviceCache for BluetoothAvailabilityManager.
	 */
	void loadPairedDevices();

	bool enoughTimeForScan(const Poco::Timestamp &startTime);

	void reportFoundDevices(const int mode, const std::map<MACAddress, std::string> &devices);

	void listen();

	void removeDevice(const DeviceID &id);

	void shipStatusOf(const BluetoothDevice &device);

	void sendNewDevice(
		const DeviceID &id,
		const MACAddress &address,
		const std::string &name);

	std::list<ModuleType> moduleTypes() const;

	DeviceID createDeviceID(const MACAddress &numMac) const;

	DeviceID createLEDeviceID(const MACAddress &numMac) const;

	Poco::RunnableAdapter<BluetoothAvailabilityManager> m_listenThread;
	Poco::Timespan m_wakeUpTime;
	Poco::Timespan m_leScanTime;
	Poco::Thread m_thread;
	std::map<DeviceID, BluetoothDevice> m_deviceList;
	Poco::FastMutex m_lock;
	Poco::FastMutex m_scanLock;
	HciInterfaceManager::Ptr m_hciManager;
	Poco::Timespan m_listenTime;
	int m_mode;
	std::map<MACAddress, std::string> m_leScanCache;
};

}
