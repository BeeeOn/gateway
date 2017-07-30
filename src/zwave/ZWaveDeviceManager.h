#pragma once

#include <map>

#include <Poco/Condition.h>
#include <Poco/Mutex.h>
#include <Poco/Nullable.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timer.h>

#include <openzwave/Notification.h>

#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "core/DeviceManager.h"
#include "hotplug/UDevListener.h"
#include "zwave/ZWaveDriver.h"
#include "zwave/ZWaveDeviceInfoRegistry.h"
#include "zwave/ZWaveNodeInfo.h"

namespace BeeeOn {

/**
 * In OpenZWave, all feedback from the Z-Wave network is sent to the
 * application via callbacks. This class allows the application to add
 * a notification callback handler. All notifications will be reported
 * to it.
 *
 * After a supported Z-Wave driver is connected, the onAdd event
 * ensures its setup and initialization via the OpenZWave library.
 */
class ZWaveDeviceManager : public DeviceManager, public UDevListener {
public:
	ZWaveDeviceManager();
	~ZWaveDeviceManager();

	void run() override;
	void stop() override;

	void onAdd(const HotplugEvent &event) override;
	void onRemove(const HotplugEvent &event) override;

	bool accept(Command::Ptr cmd) override;
	void handle(Command::Ptr cmd, Answer::Ptr answer) override;

	/**
	 * Path to save user's data (log file, xml config), store
	 * Z-Wave network config data and state.
	 */
	void setUserPath(const std::string &userPath);

	/**
	 * Path to xml config file for openzwave library.
	 */
	void setConfigPath(const std::string &configPath);

	/**
	 * For old devices, detect status changes.
	 */
	void setPollInterval(int pollInterval);

	void setDeviceInfoRegistry(Poco::SharedPtr<ZWaveDeviceInfoRegistry> factory);

	void installConfiguration();

	/**
	 * It handles notification from Z-Wave network.
	 * @param notification Provides a container for data sent via
	 * the notification
	 */
	void onNotification(const OpenZWave::Notification *notification);

private:
	/**
	 * Finding dongle path.
	 */
	std::string dongleMatch(const HotplugEvent &e);

	/**
	 * A new node value (OpenZWave::ValeID) has been added to OpenZWave's
	 * list and ZWaveNodeInfo. Before vale is added to ZWaveNodeInfo,
	 * it is checked if a given value is supported.
	 *
	 * These notifications occur after a node has been discovered.
	 *
	 * @param notification Provides a container for data sent via
	 * the notification
	 */
	void valueAdded(const OpenZWave::Notification *noticiation);

	/**
	 * A node value has been updated from the Z-Wave network and it
	 * is different from the previous value.
	 * @param notification Provides a container for data sent via
	 * the notification
	 */
	void valueChanged(const OpenZWave::Notification *noticiation);

	/**
	 * A new node has been added to OpenZWave's list. This may be
	 * due to a device being added to the Z-Wave network, or
	 * because the application is initializing itself.
	 * @param notification Provides a container for data sent via
	 * the notification
	 */
	void nodeAdded(const OpenZWave::Notification *notification);

	/**
	 * It builds DeviceID from homeID, nodeID and gatewayID.
	 *
	 * DeviceID contains:
	 *  - 8b prefix
	 *  - 16b empty
	 *  - 32b homeID
	 *  - 8b nodeID
	 */
	DeviceID buildID(uint8_t nodeID) const;
	uint8_t nodeIDFromDeviceID(const DeviceID &deviceID) const;

	/**
	 * Loads paired devices from server.
	 */
	void loadDeviceList();

	/**
	 * Modifies value.
	 */
	bool modifyValue(uint8_t nodeID,
		const ModuleID &moduleID, const std::string &value);

	void shipData(const OpenZWave::ValueID &valueID,
		const ZWaveNodeInfo &nodeInfo);

	void stopCommand(Poco::Timer &timer);

	void doNewDeviceCommand();
	void doListenCommand(
		const GatewayListenCommand::Ptr cmd, const Answer::Ptr answer);
	void doUnpairCommand(
		const DeviceUnpairCommand::Ptr cmd, const Answer::Ptr answer);
	void doDeviceAcceptCommand(
		const DeviceAcceptCommand::Ptr cmd, const Answer::Ptr answer);
	void doSetValueCommnad(
		const DeviceSetValueCommand::Ptr cmd, const Answer::Ptr answer);

	void createDevice(const DeviceID &deviceID, bool paired);

	void configureDefaultUnit(OpenZWave::ValueID &valueID);

private:
	enum State {
		/**
		 * Dongle is unplugged or Z-Wave USB device failed.
		 */
		IDLE = 0,
		/**
		 * A Z-Wave driver has been added and is ready to use. W
		 */
		DONGLE_READY = 1,
		/**
		 * All the initialization queries on a node have been completed.
		 */
		NODE_QUERIED = 2,
		/**
		 * It enables to accept new devices and communicate with them.
		 */
		LISTENING,
		/**
		 * It enables to unpair devices.
		 */
		UNPAIRING,
	};

private:
	Poco::FastMutex m_lock;
	Poco::FastMutex m_dongleLock;
	ZWaveDriver m_driver;
	std::map<uint8_t, ZWaveNodeInfo> m_beeeonDevices;
	uint32_t m_homeID;
	std::string m_userPath;
	std::string m_configPath;
	int m_pollInterval;
	State m_state;
	ZWaveDeviceInfoRegistry::Ptr m_registry;
	std::map<uint8_t, std::list<OpenZWave::ValueID>> m_zwaveNodes;
	Poco::Event m_stopEvent;

	Poco::TimerCallback<ZWaveDeviceManager> m_commandCallback;
	Poco::Timer m_commandTimer;
};

}
