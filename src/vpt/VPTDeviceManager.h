#ifndef GATEWAY_VPT_H
#define GATEWAY_VPT_H

#include <map>
#include <vector>

#include <Poco/AtomicCounter.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>
#include <Poco/Thread.h>
#include <Poco/Timespan.h>

#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "commands/NewDeviceCommand.h"
#include "core/DeviceManager.h"
#include "core/GatewayInfo.h"
#include "credentials/CredentialsStorage.h"
#include "loop/StoppableRunnable.h"
#include "model/DeviceID.h"
#include "util/CryptoConfig.h"
#include "vpt/VPTDevice.h"

namespace BeeeOn {

/**
 * @brief The class implements the work with Regulators VPT LAN v1.0. Allows us
 * to process and execute the commands from server. It means modify state
 * of proper device.
 */
class VPTDeviceManager : public DeviceManager {
public:
	/**
	 * @brief Provides searching vpt devices on network in own thread.
	 * Also takes care of thread where is the listen command performed.
	 */
	class VPTSeeker : public StoppableRunnable {
	public:
		VPTSeeker(VPTDeviceManager& parent);

		/**
		 * @brief Waits for thread to finish
		 */
		~VPTSeeker();

		void startSeeking(const Poco::Timespan& duration);

		void run() override;
		void stop() override;

	private:
		VPTDeviceManager& m_parent;

		Poco::Timespan m_duration;
		Poco::Thread m_seekerThread;
		Poco::AtomicCounter m_stop;
	};

	VPTDeviceManager();

	void run() override;
	void stop() override;

	void setRefresh(const Poco::Timespan &refresh);
	void setPingTimeout(const Poco::Timespan &timeout);
	void setHTTPTimeout(const Poco::Timespan &timeout);
	void setMaxMsgSize(int size);
	void setBlackList(const std::list<std::string>& list);
	void setPath(const std::string& path);
	void setPort(const int port);
	void setMinNetMask(const std::string& minNetMask);
	void setGatewayInfo(Poco::SharedPtr<GatewayInfo> gatewayInfo);
	void setCredentialsStorage(Poco::SharedPtr<CredentialsStorage> storage);
	void setCryptoConfig(Poco::SharedPtr<CryptoConfig> config);

protected:
	void handleGeneric(const Command::Ptr cmd, Result::Ptr result) override;

	/**
	 * @brief Gathers SensorData from devices and
	 * ships them.
	 */
	void shipFromDevices();

	/**
	 * @brief Initialized search of paired devices
	 * that were obtained by deviceList().
	 */
	void searchPairedDevices();

	/**
	 * @brief Examines if any subdevice is paired for
	 * the VPT given in the parametr.
	 */
	bool isAnySubdevicePaired(VPTDevice::Ptr device);

	/**
	 * @brief Processes the listen command.
	 */
	void doListenCommand(const GatewayListenCommand::Ptr cmd);

	/**
	 * @brief Processes the unpair command.
	 */
	void doUnpairCommand(const DeviceUnpairCommand::Ptr cmd);

	/**
	 * @brief Processes the device accept command.
	 */
	void doDeviceAcceptCommand(const DeviceAcceptCommand::Ptr cmd);

	/**
	 * @brief Sets the proper device's module to given value.
	 * @param result Will contains information about
	 * the success of the request
	 */
	void modifyValue(const DeviceSetValueCommand::Ptr cmd);

	/**
	 * @brief Returns true if any subdevice is paired.
	 * @param id DeviceID of real VPT
	 */
	bool noSubdevicePaired(const DeviceID& id) const;

	/**
	 * @brief Searchs the devices on the network.
	 */
	std::vector<VPTDevice::Ptr> seekDevices();

	/**
	 * @brief Processes a new device. It means saving the new device
	 * and informing the server about it.
	 */
	void processNewDevice(Poco::SharedPtr<VPTDevice> newDevice);

	/**
	 * @brief Tries to find password credential for VPT by the given DeviceID from
	 * the credentials storage. If the password is not found the NotFoundException
	 * is thrown.
	 * @param id DeviceID of real VPT
	 */
	std::string findPassword(const DeviceID& id);

private:
	VPTDeviceManager::VPTSeeker m_seeker;
	VPTHTTPScanner m_scanner;
	uint32_t m_maxMsgSize;
	Poco::FastMutex m_pairedMutex;

	Poco::Timespan m_refresh;
	Poco::Timespan m_httpTimeout;
	Poco::Timespan m_pingTimeout;

	/**
	 * The map maps only DeviceIDs of real VPT
	 * devices to VPTDevices.
	 */
	std::map<DeviceID, VPTDevice::Ptr> m_devices;

	Poco::SharedPtr<GatewayInfo> m_gatewayInfo;
	Poco::SharedPtr<CredentialsStorage> m_credentialsStorage;
	Poco::SharedPtr<CryptoConfig> m_cryptoConfig;
};

}

#endif
