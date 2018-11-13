#pragma once

#include <map>
#include <vector>

#include <Poco/Mutex.h>
#include <Poco/Timespan.h>

#include "belkin/BelkinWemoBulb.h"
#include "belkin/BelkinWemoDevice.h"
#include "belkin/BelkinWemoDimmer.h"
#include "belkin/BelkinWemoLink.h"
#include "belkin/BelkinWemoSwitch.h"
#include "core/AbstractSeeker.h"
#include "core/DeviceManager.h"
#include "loop/StopControl.h"
#include "model/DeviceID.h"
#include "model/RefreshTime.h"
#include "net/MACAddress.h"
#include "util/AsyncWork.h"

namespace BeeeOn {

class BelkinWemoDeviceManager : public DeviceManager {
public:
	/**
	 * @brief Provides searching belkin wemo devices on network
	 * in own thread.
	 */
	class BelkinWemoSeeker : public AbstractSeeker {
	public:
		typedef Poco::SharedPtr<BelkinWemoSeeker> Ptr;

		BelkinWemoSeeker(BelkinWemoDeviceManager& parent, const Poco::Timespan& duration);

	protected:
		void seekLoop(StopControl &control) override;

	private:
		BelkinWemoDeviceManager& m_parent;
	};

	BelkinWemoDeviceManager();

	void run() override;
	void stop() override;

	void setUPnPTimeout(const Poco::Timespan &timeout);
	void setHTTPTimeout(const Poco::Timespan &timeout);
	void setRefresh(const Poco::Timespan &refresh);

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
	 * @brief Erases the links which don't care any bulb.
	 */
	void eraseUnusedLinks();

	/**
	 * @brief Processes the device set value command.
	 */
	void doSetValueCommand(const Command::Ptr cmd);
	/**
	 * @brief Sets the proper device's module to given value.
	 * @return If the setting was successfull or not.
	 */
	bool modifyValue(const DeviceID& deviceID, const ModuleID& moduleID, const double value);

	std::vector<BelkinWemoSwitch::Ptr> seekSwitches(const StopControl& stop);
	std::vector<BelkinWemoBulb::Ptr> seekBulbs(const StopControl& stop);
	std::vector<BelkinWemoDimmer::Ptr> seekDimmers(const StopControl& stop);

	void processNewDevice(BelkinWemoDevice::Ptr newDevice);

private:
	Poco::FastMutex m_linksMutex;
	Poco::FastMutex m_pairedMutex;

	std::map<MACAddress, BelkinWemoLink::Ptr> m_links;
	std::map<DeviceID, BelkinWemoDevice::Ptr> m_devices;

	RefreshTime m_refresh;
	Poco::Timespan m_httpTimeout;
	Poco::Timespan m_upnpTimeout;
};

}
