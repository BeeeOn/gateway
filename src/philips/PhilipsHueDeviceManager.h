#ifndef GATEWAY_PHILIPS_H
#define GATEWAY_PHILIPS_H

#include <map>
#include <set>
#include <vector>

#include <Poco/AtomicCounter.h>
#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Timespan.h>

#include "core/DeviceManager.h"
#include "credentials/FileCredentialsStorage.h"
#include "loop/StoppableRunnable.h"
#include "model/DeviceID.h"
#include "net/MACAddress.h"
#include "philips/PhilipsHueBridge.h"
#include "philips/PhilipsHueBulb.h"
#include "philips/PhilipsHueListener.h"
#include "util/CryptoConfig.h"
#include "util/EventSource.h"

namespace BeeeOn {

/**
 * @brief The class implements the work with Philips Hue bulbs. Allows us
 * to process and execute the commands from server. It means modify state
 * of proper bulb.
 */
class PhilipsHueDeviceManager : public DeviceManager {
public:
	/**
	 * @brief Provides searching philips devices on network
	 * in own thread.
	 */
	class PhilipsHueSeeker : public StoppableRunnable {
	public:
		PhilipsHueSeeker(PhilipsHueDeviceManager& parent);

		void setDuration(const Poco::Timespan& duration);

		void run() override;
		void stop() override;

	private:
		PhilipsHueDeviceManager& m_parent;
		Poco::Timespan m_duration;
		Poco::AtomicCounter m_stop;
	};

	static const Poco::Timespan SEARCH_DELAY;

	PhilipsHueDeviceManager();

	void run() override;
	void stop() override;

	void setUPnPTimeout(const Poco::Timespan &timeout);
	void setHTTPTimeout(const Poco::Timespan &timeout);
	void setRefresh(const Poco::Timespan &refresh);
	void setCredentialsStorage(Poco::SharedPtr<FileCredentialsStorage> storage);
	void setCryptoConfig(Poco::SharedPtr<CryptoConfig> config);
	void setEventsExecutor(AsyncExecutor::Ptr executor);
	void registerListener(PhilipsHueListener::Ptr listener);

protected:
	bool accept(const Command::Ptr cmd) override;
	void handle(Command::Ptr cmd, Answer::Ptr answer) override;

	void refreshPairedDevices();
	void searchPairedDevices();

	/**
	 * @brief Erases the bridges which don't care any bulb.
	 */
	void eraseUnusedBridges();

	/**
	 * @brief Processes the listen command.
	 */
	void doListenCommand(const Command::Ptr cmd, const Answer::Ptr answer);

	/**
	 * @brief Processes the unpair command.
	 */
	void doUnpairCommand(const Command::Ptr cmd, const Answer::Ptr answer);

	/**
	 * @brief Processes the device accept command.
	 */
	void doDeviceAcceptCommand(const Command::Ptr cmd, const Answer::Ptr answer);

	/**
	 * @brief Sets the proper device's module to given value.
	 * @return If the setting was successfull or not.
	 */
	bool modifyValue(const DeviceID& deviceID, const ModuleID& moduleID, const double value);

	std::vector<PhilipsHueBulb::Ptr> seekBulbs();

	void authorizationOfBridge(PhilipsHueBridge::Ptr bridge);

	void processNewDevice(PhilipsHueBulb::Ptr newDevice);

	void fireBridgeStatistics(PhilipsHueBridge::Ptr bridge);
	void fireBulbStatistics(PhilipsHueBulb::Ptr bulb);

private:
	Poco::Thread m_seekerThread;
	Poco::FastMutex m_bridgesMutex;
	Poco::FastMutex m_pairedMutex;

	std::map<MACAddress, PhilipsHueBridge::Ptr> m_bridges;
	std::map<DeviceID, PhilipsHueBulb::Ptr> m_devices;
	std::set<DeviceID> m_pairedDevices;

	Poco::Timespan m_refresh;
	PhilipsHueDeviceManager::PhilipsHueSeeker m_seeker;
	Poco::Timespan m_httpTimeout;
	Poco::Timespan m_upnpTimeout;
	Poco::Event m_event;

	Poco::SharedPtr<FileCredentialsStorage> m_credentialsStorage;
	Poco::SharedPtr<CryptoConfig> m_cryptoConfig;

	EventSource<PhilipsHueListener> m_eventSource;
};

}

#endif
