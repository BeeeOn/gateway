#pragma once

#include <map>

#include <Poco/Mutex.h>
#include <Poco/Timespan.h>

#include "core/DeviceManager.h"
#include "model/DeviceID.h"
#include "net/MqttClient.h"
#include "sonoff/SonoffDevice.h"

namespace BeeeOn {

/**
 * @brief The class implements the work with Sonoff devices. Allows us
 * to process and execute the commands from server and gather
 * data from the devices.
 */
class SonoffDeviceManager : public DeviceManager {
public:
	SonoffDeviceManager();

	void run() override;
	void stop() override;
	void setMqttClient(MqttClient::Ptr mqttClient);
	void setMaxLastSeen(const Poco::Timespan &timeout);

protected:
	AsyncWork<>::Ptr startDiscovery(const Poco::Timespan &timeout) override;
	AsyncWork<std::set<DeviceID>>::Ptr startUnpair(
		const DeviceID &id,
		const Poco::Timespan &) override;
	void handleAccept(const DeviceAcceptCommand::Ptr cmd) override;

	/**
	 * @brief Processes the incoming MQTT message, which means creating
	 * a new device or sending the gathered data to the server.
	 */
	void processMQTTMessage(const MqttMessage &message);

	/**
	 * @brief Retrieves device information from JSON message received from MQTT broker.
	 * @return Pair of device id and device name.
	 */
	std::pair<DeviceID, std::string> retrieveDeviceInfo(const std::string &message) const;

	/**
	 * @brief Creates instance of sonoff device according to device name and
	 * appends it into m_devices.
	 */
	void createNewDevice(const std::pair<DeviceID, std::string>& deviceInfo);

private:
	Poco::FastMutex m_devicesMutex;
	std::map<DeviceID, SonoffDevice::Ptr> m_devices;

	MqttClient::Ptr m_mqttClient;
	Poco::Timespan m_maxLastSeen;
};
}
