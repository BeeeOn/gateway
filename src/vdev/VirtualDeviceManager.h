#ifndef BEEEON_VIRTUAL_DEVICE_MANAGER_H
#define BEEEON_VIRTUAL_DEVICE_MANAGER_H

#include <queue>
#include <string>

#include <Poco/SharedPtr.h>
#include <Poco/Timestamp.h>
#include <Poco/Util/IniFileConfiguration.h>

#include "core/DeviceManager.h"
#include "vdev/VirtualDevice.h"

namespace BeeeOn {

/**
 * Represents entry in a calendar. It contains time when entry
 * was inserted into the calendar and information about device.
 *
 * Note: Calendar serves for planning of data sending from modules.
 */
class VirtualDeviceEntry {
public:
	VirtualDeviceEntry(VirtualDevice::Ptr device);

	/**
	* Sets time when entry was inserted into a calendar.
	*/
	void setInserted(const Poco::Timestamp &t);

	/**
	* Returns time when entry was inserted into a calendar.
	*/
	Poco::Timestamp inserted() const;

	/**
	* Returns time when entry (device) will be activated
	* (when data will be sent).
	 *
	* activationTime = timeInserted + refreshTime
	*/
	Poco::Timestamp activationTime() const;

	/**
	* Returns information about device.
	*/
	VirtualDevice::Ptr device() const;

private:
	Poco::Timestamp m_inserted;
	VirtualDevice::Ptr m_device;
};

/**
 * Ensures comparison of entries in a calendar.
 */
class VirtualDeviceEntryComparator {
public:
	/**
	* Returns entry with lower activation time.
	*/
	bool lessThan(const VirtualDeviceEntry &deviceQueue1,
		const VirtualDeviceEntry &deviceQueue2) const;

	/**
	* Returns entry with lower activation time.
	*/
	bool operator ()(const VirtualDeviceEntry &deviceQueue1,
		const VirtualDeviceEntry &deviceQueue2) const
	{
		return lessThan(deviceQueue1, deviceQueue2);
	}
};

/**
 * Ensures configuration of virtual devices from configuration file
 * virtual-devices.ini and it is able to send NewDeviceCommand to CommandDispatcher
 * when device attempts to pair.
 *
 * It can send values from modules of registered devices.
 *
 * It also ensures reaction to commands sent from server:
 *
 * - GatewayListenCommand, DeviceAcceptCommand - device attempts to pair
 * - DeviceSetValueCommand - modification of module value
 * - DeviceUnpairCommand - device attempts to unpair
 */
class VirtualDeviceManager : public DeviceManager {
public:
	VirtualDeviceManager();

	void run() override;
	void stop() override;

	/**
	 * Sets path to configuration file.
	 */
	void setConfigFile(const std::string &path);

	/**
	 * Loads setting of virtual devices from configuration file
	 * and stores this information.
	 */
	void installVirtualDevices();

	/**
	 * Inserts item to map of virtual devices if there is no virtual
	 * device with given identifier.
	 */
	void registerDevice(const VirtualDevice::Ptr virtualDevice);

	/**
	 * Ensures sending of NewDeviceCommand to CommandDispatcher.
	 */
	void dispatchNewDevice(VirtualDevice::Ptr device);

	/**
	 * Processes information about virtual device loaded from configuration
	 * file.
	 */
	VirtualDevice::Ptr parseDevice(
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> cfg);

	/**
	 * Processes information about virtual module loaded from configuration
	 * file.
	 */
	VirtualModule::Ptr parseModule(
		Poco::AutoPtr<Poco::Util::AbstractConfiguration> cfg,
		const ModuleID &moduleID);

	/**
	 * Sets devices as paired according to the device list sent from server.
	 */
	void setPairedDevices();

	/**
	 * Plans devices that are in a map of virtual devices and are paired.
	 */
	void scheduleAllEntries();

	/**
	 * Checks if a queue of virtual devices is empty.
	 */
	bool isEmptyQueue();

	/**
	 * Sets time when an entry was inserted into a queue and pushes
	 * this entry to the queue.
	 */
	void scheduleEntryUnlocked(VirtualDeviceEntry entry);

	/**
	 * Logs information about loaded virtual devices and modules.
	 * Detail of information can be selected from possibilities:
	 * information, debug, trace.
	 */
	void logDeviceParsed(VirtualDevice::Ptr device);

protected:
	void handle(Command::Ptr cmd, Answer::Ptr answer) override;

	/**
	 * Reacts to GatewayListenCommand. It sends NewDeviceCommand if
	 * device is not paired.
	 */
	void doListenCommand(const GatewayListenCommand::Ptr,
		const Answer::Ptr answer);

	/**
	 * Reacts to DeviceAcceptCommand. Device has to be stored in map
	 * of virtual devices and it has to be unpaired. If these conditions
	 * are fulfilled, method inserts device into a calendar, it sets device
	 * as paired and it plans next activation (data generation) of this device.
	 */
	void doDeviceAcceptCommand(const DeviceAcceptCommand::Ptr cmd,
		const Answer::Ptr answer);

	/**
	* Reacts to DeviceSetValueCommand. Device has to be in map of
	* virtual devices, it has to be sensor and reaction has to be
	* set to success.
	*/
	void doSetValueCommand(const DeviceSetValueCommand::Ptr cmd,
		const Answer::Ptr answer);

	/**
	* Reacts to DeviceUnpairCommand. Device has to be in map of
	* virtual devices and it has to be paired.
	*/
	void doUnpairCommand(const DeviceUnpairCommand::Ptr cmd,
		const Answer::Ptr answer);

private:
	std::map<DeviceID, VirtualDevice::Ptr> m_virtualDevicesMap;
	std::string m_configFile;
	std::priority_queue<VirtualDeviceEntry,
		std::vector<VirtualDeviceEntry>,
		VirtualDeviceEntryComparator> m_virtualDeviceQueue;
	Poco::Event m_event;
	bool m_requestDeviceList;
	Poco::FastMutex m_lock;
};

}

#endif
