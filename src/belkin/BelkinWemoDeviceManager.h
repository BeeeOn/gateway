#ifndef GATEWAY_BELKINWEMO_H
#define GATEWAY_BELKINWEMO_H

#include <map>
#include <set>
#include <vector>

#include <Poco/AtomicCounter.h>
#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/Thread.h>
#include <Poco/Timespan.h>

#include "belkin/BelkinWemoBulb.h"
#include "belkin/BelkinWemoDevice.h"
#include "belkin/BelkinWemoDimmer.h"
#include "belkin/BelkinWemoLink.h"
#include "belkin/BelkinWemoSwitch.h"
#include "core/DeviceManager.h"
#include "loop/StoppableRunnable.h"
#include "model/DeviceID.h"
#include "net/MACAddress.h"


namespace BeeeOn {

class BelkinWemoDeviceManager : public DeviceManager {
public:
	/**
	 * @brief Provides searching belkin wemo devices on network
	 * in own thread.
	 */
	class BelkinWemoSeeker : public StoppableRunnable {
	public:
		BelkinWemoSeeker(BelkinWemoDeviceManager& parent);

		void setDuration(const Poco::Timespan& duration);

		void run() override;
		void stop() override;

	private:
		BelkinWemoDeviceManager& m_parent;
		Poco::Timespan m_duration;
		Poco::AtomicCounter m_stop;
	};

	BelkinWemoDeviceManager();

	void run() override;
	void stop() override;

	void setUPnPTimeout(const Poco::Timespan &timeout);
	void setHTTPTimeout(const Poco::Timespan &timeout);
	void setRefresh(const Poco::Timespan &refresh);

protected:
	bool accept(const Command::Ptr cmd) override;
	void handle(Command::Ptr cmd, Answer::Ptr answer) override;

	void refreshPairedDevices();
	void searchPairedDevices();

	/**
	 * @brief Erases the links which don't care any bulb.
	 */
	void eraseUnusedLinks();

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

	std::vector<BelkinWemoSwitch::Ptr> seekSwitches();
	std::vector<BelkinWemoBulb::Ptr> seekBulbs();
	std::vector<BelkinWemoDimmer::Ptr> seekDimmers();

	void processNewDevice(BelkinWemoDevice::Ptr newDevice);

private:
	Poco::Thread m_seekerThread;
	Poco::FastMutex m_linksMutex;
	Poco::FastMutex m_pairedMutex;

	std::map<MACAddress, BelkinWemoLink::Ptr> m_links;
	std::map<DeviceID, BelkinWemoDevice::Ptr> m_devices;
	std::set<DeviceID> m_pairedDevices;

	Poco::Timespan m_refresh;
	BelkinWemoDeviceManager::BelkinWemoSeeker m_seeker;
	Poco::Timespan m_httpTimeout;
	Poco::Timespan m_upnpTimeout;
	Poco::Event m_event;
};

}

#endif
