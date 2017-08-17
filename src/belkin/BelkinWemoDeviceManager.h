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

#include "belkin/BelkinWemoSwitch.h"
#include "core/DeviceManager.h"
#include "loop/StoppableRunnable.h"
#include "model/DeviceID.h"


namespace BeeeOn {

class BelkinWemoDeviceManager : public DeviceManager {
public:
	/*
	 * @brief Provides searching belkin wemo devices on network
	 * in own thread.
	 */
	class BelkinWemoSeeker : public StoppableRunnable {
	public:
		BelkinWemoSeeker(BelkinWemoDeviceManager& parent);

		void setDuration(const Poco::Timespan& duration);

		void run() override;
		void stop() override;

		int divRoundUp(const int x, const int y);

	private:
		BelkinWemoDeviceManager& m_parent;
		Poco::Timespan m_duration;
		Poco::AtomicCounter m_stop;
	};

	BelkinWemoDeviceManager();

	void run() override;
	void stop() override;

	void setUPnPTimeout(int secs);
	void setHTTPTimeout(int secs);
	void setRefresh(int secs);

protected:
	bool accept(const Command::Ptr cmd) override;
	void handle(Command::Ptr cmd, Answer::Ptr answer) override;

	void refreshPairedDevices();

	/*
	 * @brief Processes the listen command.
	 */
	void doListenCommand(const Command::Ptr cmd, const Answer::Ptr answer);

	/*
	 * @brief Processes the unpair command.
	 */
	void doUnpairCommand(const Command::Ptr cmd, const Answer::Ptr answer);

	/*
	 * @brief Processes the device accept command.
	 */
	void doDeviceAcceptCommand(const Command::Ptr cmd, const Answer::Ptr answer);

	/*
	 * @brief Sets the proper device's module to given value.
	 * @return If the setting was successfull or not.
	 */
	bool modifyValue(const DeviceID& deviceID, const ModuleID& moduleID, const double value);

	std::vector<BelkinWemoSwitch> seekSwitches();

	void processNewDevice(BelkinWemoSwitch& newDevice);

private:
	Poco::Thread m_seekerThread;
	Poco::FastMutex m_pairedMutex;

	std::map<DeviceID, BelkinWemoSwitch> m_switches;
	std::set<DeviceID> m_pairedDevices;

	Poco::Timespan m_refresh;
	BelkinWemoDeviceManager::BelkinWemoSeeker m_seeker;
	Poco::Timespan m_httpTimeout;
	Poco::Timespan m_upnpTimeout;
	Poco::Event m_event;
};

}

#endif
