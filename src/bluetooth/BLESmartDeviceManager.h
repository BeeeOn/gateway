#pragma once

#include <map>
#include <vector>

#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Timespan.h>
#include <Poco/UUID.h>

#include "bluetooth/BLESmartDevice.h"
#include "bluetooth/HciInterface.h"
#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceSetValueCommand.h"
#include "core/AbstractSeeker.h"
#include "core/DongleDeviceManager.h"
#include "core/PollingKeeper.h"
#include "loop/StopControl.h"
#include "model/DeviceID.h"
#include "model/RefreshTime.h"
#include "net/MACAddress.h"
#include "util/AsyncWork.h"

namespace BeeeOn {

/**
 * @brief The class implements the work with Bluetooth Low Energy devices.
 * Allows us to process and execute the commands from server and gather
 * data from the devices.
 */
class BLESmartDeviceManager : public DongleDeviceManager {
public:
	static const Poco::UUID CHAR_MODEL_NUMBER;

	/**
	 * @brief Provides searching BLE devices on network
	 * in own thread.
	 */
	class BLESmartSeeker : public AbstractSeeker {
	public:
		typedef Poco::SharedPtr<BLESmartSeeker> Ptr;

		BLESmartSeeker(
			BLESmartDeviceManager& parent,
			const Poco::Timespan& duration);

	protected:
		void seekLoop(StopControl &control) override;

	private:
		BLESmartDeviceManager& m_parent;
	};

	BLESmartDeviceManager();

	void dongleAvailable() override;

	/**
	 * @brief Recognizes compatible dongle by testing HotplugEvent property
	 * as <code>bluetooth.BEEEON_DONGLE == bluetooth</code>.
	 */
	std::string dongleMatch(const HotplugEvent &e) override;

	/**
	 * @brief Erases all instances of the device.
	 */
	void dongleFailed(const FailDetector &dongleStatus) override;

	/**
	 * @brief Erases all instances of the device.
	 */
	bool dongleMissing() override;
	void stop() override;

	void setDevicePoller(DevicePoller::Ptr poller);
	void setScanTimeout(const Poco::Timespan &timeout);
	void setDeviceTimeout(const Poco::Timespan &timeout);
	void setRefresh(const Poco::Timespan &refresh);
	void setNumberOfExaminationThreads(const int numberOfExaminationThreads);
	void setHciManager(HciInterfaceManager::Ptr manager);

protected:
	/**
	 * @brief Wakes up the main thread.
	 */
	void notifyDongleRemoved() override;
	void handleAccept(const DeviceAcceptCommand::Ptr cmd) override;
	AsyncWork<>::Ptr startDiscovery(const Poco::Timespan &timeout) override;
	AsyncWork<std::set<DeviceID>>::Ptr startUnpair(
		const DeviceID &id,
		const Poco::Timespan &timeout) override;
	AsyncWork<double>::Ptr startSetValue(
		const DeviceID &id,
		const ModuleID &module,
		const double value,
		const Poco::Timespan &timeout) override;

	/**
	 * @brief Clears m_devices.
	 */
	void eraseAllDevices();

	/**
	 * @brief Processes the asynchronous data of the paired device.
	 * If the message is correct it is already shipped to server.
	 */
	void processAsyncData(
		const MACAddress& address,
		std::vector<unsigned char> &data);

	/**
	 * @brief Tries to find not found paired devices. Each found device
	 * is added to parametr devices and attribute m_devices.
	 */
	void seekPairedDevices();

	/**
	 * @brief Seeks for new devices on Bluetooth LE network.
	 *
	 * The process of identification device consists of two steps.
	 * In the first step, it is determined wheter the device name
	 * is in set of names of potentially supported device. If so,
	 * then the model id of device is obtained according to which
	 * the device is identified.
	 */
	void seekDevices(
		std::vector<BLESmartDevice::Ptr>& foundDevices,
		const StopControl& stop);

	/**
	 * @brief Splits the map of devices into smaller maps whose number
	 * is determined by the attribute m_numberOfExaminationThreads.
	 * These maps of devices are then examines in the seperated threads.
	 */
	void threadedExaminationOfDevices(
		const std::map<MACAddress, std::string>& devices,
		Poco::FastMutex& foundDevicesMutex,
		std::vector<BLESmartDevice::Ptr>& foundDevices,
		const StopControl& stop);

	/**
	 * @brief Finds out if the given devices are supported. In the
	 * positive case, a specific instance of the device is created
	 * for it.
	 */
	void examineBatchOfDevices(
		const std::map<MACAddress, std::string>& devices,
		Poco::FastMutex& foundDevicesMutex,
		std::vector<BLESmartDevice::Ptr>& foundDevices,
		const StopControl& stop);

	/**
	 * @brief Creates BLE device based on its Model ID.
	 */
	BLESmartDevice::Ptr createDevice(const MACAddress& address) const;

	void processNewDevice(BLESmartDevice::Ptr newDevice);

private:
	Poco::FastMutex m_devicesMutex;
	std::map<DeviceID, BLESmartDevice::Ptr> m_devices;
	Poco::SharedPtr<HciInterface::WatchCallback> m_watchCallback;

	PollingKeeper m_pollingKeeper;
	Poco::Timespan m_scanTimeout;
	Poco::Timespan m_deviceTimeout;
	RefreshTime m_refresh;
	uint32_t m_numberOfExaminationThreads;
	HciInterfaceManager::Ptr m_hciManager;
	HciInterface::Ptr m_hci;
};

}
