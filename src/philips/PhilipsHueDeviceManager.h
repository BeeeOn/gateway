#ifndef GATEWAY_PHILIPS_H
#define GATEWAY_PHILIPS_H

#include <map>
#include <vector>

#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Timespan.h>

#include "core/DeviceManager.h"
#include "credentials/FileCredentialsStorage.h"
#include "loop/StoppableRunnable.h"
#include "loop/StopControl.h"
#include "model/DeviceID.h"
#include "net/MACAddress.h"
#include "philips/PhilipsHueBridge.h"
#include "philips/PhilipsHueBulb.h"
#include "philips/PhilipsHueListener.h"
#include "util/AsyncWork.h"
#include "util/CryptoConfig.h"
#include "util/EventSource.h"
#include "util/Joiner.h"

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
	class PhilipsHueSeeker : public StoppableRunnable, public AsyncWork<> {
	public:
		typedef Poco::SharedPtr<PhilipsHueSeeker> Ptr;

		PhilipsHueSeeker(PhilipsHueDeviceManager& parent);

		void startSeeking(const Poco::Timespan& duration);

		void run() override;
		void stop() override;
		bool tryJoin(const Poco::Timespan &timeout) override;
		void cancel() override;

	private:
		PhilipsHueDeviceManager& m_parent;
		Poco::Timespan m_duration;
		StopControl m_stopControl;
		Poco::Thread m_seekerThread;
		Joiner m_joiner;
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
	void handleGeneric(const Command::Ptr cmd, Result::Ptr result) override;
	void handleAccept(const DeviceAcceptCommand::Ptr cmd) override;
	AsyncWork<>::Ptr startDiscovery(const Poco::Timespan &timeout) override;
	AsyncWork<std::set<DeviceID>>::Ptr startUnpair(
		const DeviceID &id,
		const Poco::Timespan &timeout) override;

	void refreshPairedDevices();
	void searchPairedDevices();

	/**
	 * @brief Erases the bridges which don't care any bulb.
	 */
	void eraseUnusedBridges();

	/**
	 * @brief Processes the set value command.
	 */
	void doSetValueCommand(const Command::Ptr cmd);

	/**
	 * @brief Sets the proper device's module to given value.
	 * @return If the setting was successfull or not.
	 */
	bool modifyValue(const DeviceID& deviceID, const ModuleID& moduleID, const double value);

	std::vector<PhilipsHueBulb::Ptr> seekBulbs(const StopControl& stop);

	void authorizationOfBridge(PhilipsHueBridge::Ptr bridge);

	void processNewDevice(PhilipsHueBulb::Ptr newDevice);

	void fireBridgeStatistics(PhilipsHueBridge::Ptr bridge);
	void fireBulbStatistics(PhilipsHueBulb::Ptr bulb);

private:
	Poco::FastMutex m_bridgesMutex;
	Poco::FastMutex m_pairedMutex;

	std::map<MACAddress, PhilipsHueBridge::Ptr> m_bridges;
	std::map<DeviceID, PhilipsHueBulb::Ptr> m_devices;

	Poco::Timespan m_refresh;
	Poco::Timespan m_httpTimeout;
	Poco::Timespan m_upnpTimeout;

	Poco::SharedPtr<FileCredentialsStorage> m_credentialsStorage;
	Poco::SharedPtr<CryptoConfig> m_cryptoConfig;

	EventSource<PhilipsHueListener> m_eventSource;
};

}

#endif
