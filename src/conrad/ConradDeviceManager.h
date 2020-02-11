#pragma once

#include <string>

#include <Poco/Timespan.h>
#include <Poco/JSON/Object.h>

#include "core/DeviceManager.h"
#include "conrad/ConradDevice.h"
#include "conrad/ConradListener.h"
#include "conrad/FHEMClient.h"
#include "model/DeviceID.h"
#include "util/AsyncExecutor.h"
#include "util/BlockingAsyncWork.h"
#include "util/EventSource.h"
#include "util/JsonUtil.h"

namespace BeeeOn {

/**
 * @brief The class implements the work with Conrad devices. Allows us
 * to process and execute the commands from server and gather
 * data from the devices. It communicates with the Conrad devices
 * using FHEM server. To communicate with FHEM server the class using
 * FHEMClient which communicates with the FHEM server over telnet.
 */
class ConradDeviceManager : public DeviceManager {
public:
	ConradDeviceManager();

	void run() override;
	void stop() override;

	void setFHEMClient(FHEMClient::Ptr fhemClient);
	void setEventsExecutor(AsyncExecutor::Ptr executor);

	void registerListener(ConradListener::Ptr listener);

protected:
	AsyncWork<>::Ptr startDiscovery(const Poco::Timespan &timeout) override;
	AsyncWork<std::set<DeviceID>>::Ptr startUnpair(
		const DeviceID &id,
		const Poco::Timespan &) override;
	void handleAccept(const DeviceAcceptCommand::Ptr cmd) override;

	/**
	 * @brief Processes the incoming event, which means creating
	 * a new device or sending the gathered data to the server.
	 */
	void processEvent(const Poco::JSON::Object::Ptr event);

	/**
	 * @brief Creates instance of Conrad device appends it into m_devices.
	 */
	void createNewDeviceUnlocked(const DeviceID &deviceID, const std::string &type);

	/**
	* @brief Transforms received/sent message into ConradEvent and fires it.
	*/
	void fireMessage(DeviceID const &deviceID, const Poco::JSON::Object::Ptr message);

private:
	Poco::FastMutex m_devicesMutex;
	std::map<DeviceID, ConradDevice::Ptr> m_devices;

	FHEMClient::Ptr m_fhemClient;
	EventSource<ConradListener> m_eventSource;
};

}
