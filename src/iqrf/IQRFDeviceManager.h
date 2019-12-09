#pragma once

#include <set>
#include <string>

#include <Poco/AtomicCounter.h>
#include <Poco/SharedPtr.h>
#include <Poco/Util/Timer.h>

#include "core/DongleDeviceManager.h"
#include "core/PollingKeeper.h"
#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "iqrf/DPAMessage.h"
#include "iqrf/DPAProtocol.h"
#include "iqrf/IQRFDevice.h"
#include "iqrf/IQRFEventFirer.h"
#include "iqrf/IQRFListener.h"
#include "iqrf/IQRFMqttConnector.h"
#include "model/RefreshTime.h"

namespace BeeeOn {

/**
 * @brief IQRFDeviceManager implements work with IQRF devices. IQRF Daemon
 * (third party application) is used for communication with IQRF devices.
 * For the communication with IQRF Daemon, IQRFConnector that allows to
 * send/receive JSON message is used. Obtaining of measured data is
 * implemented using LambdaTimerTask.
 */
class IQRFDeviceManager : public DongleDeviceManager {
public:
	IQRFDeviceManager();

	/**
	 * @brief Registration of supported protocols.
	 */
	void registerDPAProtocol(DPAProtocol::Ptr protocol);

	/**
	 * @brief The time during which response should be received.
	 */
	void setReceiveTimeout(const Poco::Timespan &timeout);

	/**
	 * @brief Refresh time between obtaining of measured values from
	 * IQRF devices.
	 */
	void setRefreshTime(const Poco::Timespan &refresh);

	/**
	 * @brief Refresh time between obtaining peripheral info
	 * (battery, RSSI) from IQRF devices.
	 */
	void setRefreshTimePeripheralInfo(const Poco::Timespan &refresh);

	/**
	 * @brief Sets the timeout of waiting for next attempt to communicate
	 * with IQRF devices. Timeout must be at least 1 ms.
	 */
	void setIQRFDevicesRetryTimeout(const Poco::Timespan &timeout);

	/**
	 * @brief If this parameter is set to true (bool value in decimal
	 * or string notation), coordinator is reset.
	 */
	void setCoordinatorReset(const std::string & reset);

	void setMqttConnector(IQRFMqttConnector::Ptr connector);
	void setDevicePoller(DevicePoller::Ptr poller);

	void setEventsExecutor(AsyncExecutor::Ptr executor);
	void registerListener(IQRFListener::Ptr listener);

	/**
	 * @brief Recognizes compatible dongle by testing HotplugEvent
	 * property as <code>iqrf.BEEEON_DONGLE = iqrf</code>.
	 */
	std::string dongleMatch(const HotplugEvent &e) override;
	void dongleAvailable() override;
	void stop() override;

protected:
	void handleAccept(const DeviceAcceptCommand::Ptr cmd) override;
	AsyncWork<>::Ptr startDiscovery(const Poco::Timespan &timeout) override;
	AsyncWork<std::set<DeviceID>>::Ptr startUnpair(
		const DeviceID &id,
		const Poco::Timespan &timeout) override;

	/**
	 * @brief List of tasks is cleared for each device, that is in m_devices
	 * and each device is removed from the list.
	 */
	void notifyDongleRemoved() override;
	void dongleFailed(const FailDetector &dongleStatus);

	void eraseAllDevices();


private:
	/**
	 * @brief Requests for the paired devices from IQRF network.
	 */
	std::set<uint8_t> obtainBondedNodes(const Poco::Timespan &methodTimeout);
	std::set<uint8_t> obtainNonSynchronizedNodes(
		const std::set<uint8_t> &bondedNodes);

	/**
	 * @brief Scans registered devices from coordinator and synchronizes
	 * them with paired cache. The devices that are paired in device
	 * cache and they are missing in list of bonded devices are skipped.
	 */
	void syncBondedNodes(const Poco::Timespan &methodTimeout);

	/**
	 * @return String contains all supported protocols separated by comma
	 * <code>GeneralProtocol, IQHomeProtocol</code>.
	 */
	std::string supportedProtocolsToString() const;

	/**
	 * @brief Sends new obtained IQRF device from IQRF network to the server.
	 */
	void newDevice(IQRFDevice::Ptr dev);

	/**
	 * @brief Detects supported protocol for communication with IQRF device.
	 *
	 * @return Supported protocol for communication with IQRF devices.
	 * If device is unavailable or the device communicates using unsupported
	 * protocol, nullptr is returned.
	 */
	DPAProtocol::Ptr detectNodeProtocol(
		DPAMessage::NetworkAddress node,
		const Poco::Timespan &maxMethodTimeout);

	/**
	 * @brief Obtaining of the necessary information about IQRF devices.
	 * The necessary information about IQRF devices are:
	 *
	 *  - Node ID
	 *  - MID
	 *  - Protocol ID
	 *  - List of supported ModuleIDs
	 *  - HWPID
	 *  - Vendor name
	 *  - Product name
	 *
	 * @return IQRFDevice and DeviceID with the necessary information.
	 */
	std::map<DeviceID, IQRFDevice::Ptr> obtainDeviceInfo(
		const Poco::Timespan &methodTimeout,
		const std::set<uint8_t> &nodes);

	/**
	 * @brief Obtain device info.
	 *
	 * @throws Poco::IllegalStateException when the number of attempts
	 * is exhausted
	 */
	IQRFDevice::Ptr tryObtainDeviceInfo(
		DPAMessage::NetworkAddress node,
		const Poco::Timespan &methodTimeout);

	/**
	 * @brief Bonds the new IQRF device to the IQRF network.
	 *
	 * Bonding process is composed of two parts. The first part is bonding
	 * of a new device (add new device to IQRF network) and the second part
	 * is discovering of new device (create IQRF network topology).
	 */
	IQRFDevice::Ptr bondNewDevice(
		const Poco::Timespan &timeout);

	/**+
	 * @brief Resets IQRF USB device (IQRF Coordinator). The method clears
	 * list of bonded devices.
	 */
	void coordinatorResetProcess();

private:
	Poco::FastMutex m_lock;
	std::vector<DPAProtocol::Ptr> m_dpaProtocols;
	std::map<DeviceID, IQRFDevice::Ptr> m_devices;
	PollingKeeper m_pollingKeeper;

	IQRFMqttConnector::Ptr m_connector;
	RefreshTime m_refreshTime;
	RefreshTime m_refreshTimePeripheralInfo;
	bool m_coordinatorReset;
	Poco::Timespan m_receiveTimeout;
	Poco::Timespan m_devicesRetryTimeout;
	Poco::AtomicCounter m_bondingMode;

	IQRFEventFirer m_eventFirer;
};

}
