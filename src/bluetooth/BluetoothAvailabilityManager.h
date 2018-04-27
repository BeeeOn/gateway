#ifndef BEEEON_BLUETOOTH_AVAILABILITY_MANAGER_H
#define BEEEON_BLUETOOTH_AVAILABILITY_MANAGER_H

#include <list>
#include <map>
#include <string>

#include <Poco/Event.h>
#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <Poco/RunnableAdapter.h>
#include <Poco/Thread.h>
#include <Poco/Timespan.h>

#include "bluetooth/BluetoothDevice.h"
#include "bluetooth/BluetoothListener.h"
#include "bluetooth/HciInterface.h"
#include "core/DongleDeviceManager.h"
#include "model/DeviceID.h"
#include "model/SensorData.h"
#include "util/EventSource.h"
#include "util/PeriodicRunner.h"

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

	std::string dongleMatch(const HotplugEvent &e) override;

	void stop() override;

	Poco::Timespan detectAll(const HciInterface &hci);

	/**
	 * How often will be scan of paired devices
	 */
	void setWakeUpTime(const Poco::Timespan &time);

	void setModes(const std::list<std::string> &modes);

	void doDeviceAcceptCommand(const Command::Ptr &cmd);

	void doUnpairCommand(const Command::Ptr &cmd);

	void doListenCommand(const Command::Ptr &cmd);

	/**
	 * Set interval of periodic bluetooth statistics generation.
	 */
	void setStatisticsInterval(const Poco::Timespan &interval);

	/**
	 * Set HciInterfaceManager implementation.
	 */
	void setHciManager(HciInterfaceManager::Ptr manager);

	/**
	 * Set executor for delivering events.
	 */
	void setExecutor(Poco::SharedPtr<AsyncExecutor> executor);

	/**
	 * Register listener of bluetooth events.
	 */
	void registerListener(BluetoothListener::Ptr listener);

protected:
	void handleGeneric(const Command::Ptr cmd, Result::Ptr result) override;
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

	void fetchDeviceList();

	bool enoughTimeForScan(const Poco::Timestamp &startTime);

	void reportFoundDevices(const int mode, const std::map<MACAddress, std::string> &devices);

	void listen();

	void addDevice(const DeviceID &id);

	bool hasDevice(const DeviceID &id);

	void removeDevice(const DeviceID &id);

	void shipStatusOf(const BluetoothDevice &device);

	void sendNewDevice(const DeviceID &id, const std::string &name);

	std::list<ModuleType> moduleTypes() const;

	DeviceID createDeviceID(const MACAddress &numMac) const;

	DeviceID createLEDeviceID(const MACAddress &numMac) const;

	Poco::RunnableAdapter<BluetoothAvailabilityManager> m_listenThread;
	Poco::Timespan m_wakeUpTime;
	Poco::Thread m_thread;
	std::map<DeviceID, BluetoothDevice> m_deviceList;
	Poco::Event m_stopEvent;
	Poco::FastMutex m_lock;
	Poco::FastMutex m_scanLock;
	HciInterfaceManager::Ptr m_hciManager;
	PeriodicRunner m_statisticsRunner;
	EventSource<BluetoothListener> m_eventSource;
	Poco::Timespan m_listenTime;
	int m_mode;
	std::map<MACAddress, std::string> m_leScanCache;
};

}

#endif //BEEEON_BLUETOOTH_AVAILABILITY_MANAGER_H
