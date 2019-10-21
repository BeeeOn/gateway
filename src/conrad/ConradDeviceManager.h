#pragma once

#include <string>

#include <Poco/Timespan.h>
#include <Poco/JSON/Object.h>
#include <Poco/URI.h>

#include "core/DeviceManager.h"
#include "conrad/ConradDevice.h"
#include "model/DeviceID.h"
#include "util/BlockingAsyncWork.h"
#include "util/JsonUtil.h"

namespace BeeeOn {

/**
 * @brief The class implements the work with Conrad devices. Allows us
 * to process and execute the commands from server and gather
 * data from the devices. This class requires ZMQ interface that is
 * available at https://github.com/pepa-cz/conrad-interface.
 */
class ConradDeviceManager : public DeviceManager {
public:
	ConradDeviceManager();

	void run() override;
	void stop() override;

	/**
	 * @brief Sets command ZMQ interface.
	 */
	void setCmdZmqIface(const std::string &cmdZmqIface);

	/**
	 * @brief Sets event ZMQ interface.
	 */
	void setEventZmqIface(const std::string &eventZmqIface);

protected:
	AsyncWork<>::Ptr startDiscovery(const Poco::Timespan &timeout) override;
	AsyncWork<std::set<DeviceID>>::Ptr startUnpair(
		const DeviceID &id,
		const Poco::Timespan &) override;
	void handleAccept(const DeviceAcceptCommand::Ptr cmd) override;

	/**
	 * @brief Send request via ZMQ conrad interface and returns response.
	 */
	Poco::JSON::Object::Ptr sendCmdRequest(const std::string &request);

	/**
	 * @brief Processes the incoming ZMQ message, which means creating
	 * a new device or sending the gathered data to the server.
	 */
	void processMessage(const std::string &message);

	/**
	 * @brief Creates instance of Conrad device appends it into m_devices.
	 */
	void createNewDeviceUnlocked(const DeviceID &deviceID, const std::string &type);

private:
	Poco::URI m_cmdZmqIface;
	Poco::URI m_eventZmqIface;

	Poco::FastMutex m_devicesMutex;
	std::map<DeviceID, ConradDevice::Ptr> m_devices;
};

}
