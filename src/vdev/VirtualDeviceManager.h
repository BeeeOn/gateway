#pragma once

#include <queue>
#include <string>

#include <Poco/SharedPtr.h>
#include <Poco/Timestamp.h>
#include <Poco/Util/IniFileConfiguration.h>

#include "core/DeviceManager.h"
#include "core/PollingKeeper.h"
#include "vdev/VirtualDevice.h"

namespace BeeeOn {

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

	void setDevicePoller(DevicePoller::Ptr poller);

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
	 * Plans devices that are in a map of virtual devices and are paired.
	 */
	void scheduleAllEntries();

	/**
	 * Logs information about loaded virtual devices and modules.
	 * Detail of information can be selected from possibilities:
	 * information, debug, trace.
	 */
	void logDeviceParsed(VirtualDevice::Ptr device);

	/**
	 * @brief Reschedule virtual devices after updating the remote status.
	 */
	void handleRemoteStatus(
		const DevicePrefix &prefix,
		const std::set<DeviceID> &devices,
		const DeviceStatusHandler::DeviceValues &values) override;

protected:
	void handleGeneric(const Command::Ptr cmd, Result::Ptr result) override;

	/**
	 * Reacts to GatewayListenCommand. It sends NewDeviceCommand if
	 * device is not paired.
	 */
	void doListenCommand(const GatewayListenCommand::Ptr);

	/**
	 * Reacts to DeviceAcceptCommand. Device has to be stored in map
	 * of virtual devices and it has to be unpaired. If these conditions
	 * are fulfilled, method inserts device into a calendar, it sets device
	 * as paired and it plans next activation (data generation) of this device.
	 */
	void doDeviceAcceptCommand(const DeviceAcceptCommand::Ptr cmd);

	/**
	* Reacts to DeviceSetValueCommand. Device has to be in map of
	* virtual devices, it has to be sensor and reaction has to be
	* set to success.
	*/
	void doSetValueCommand(const DeviceSetValueCommand::Ptr cmd);

	/**
	* Reacts to DeviceUnpairCommand. Device has to be in map of
	* virtual devices and it has to be paired.
	*/
	void doUnpairCommand(const DeviceUnpairCommand::Ptr cmd);

private:
	std::map<DeviceID, VirtualDevice::Ptr> m_virtualDevicesMap;
	std::string m_configFile;
	PollingKeeper m_pollingKeeper;
	bool m_requestDeviceList;
	Poco::FastMutex m_lock;
};

}
