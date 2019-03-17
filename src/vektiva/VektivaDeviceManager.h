#pragma once

#include <vector>

#include <Poco/Mutex.h>

#include "core/AbstractSeeker.h"
#include "core/DeviceManager.h"
#include "model/DeviceID.h"
#include "net/MqttClient.h"
#include "net/MACAddress.h"
#include "util/BlockingAsyncWork.h"
#include "vektiva/VektivaSmarwi.h"

namespace BeeeOn {

/**
 * @brief Vektiva device manager provides an easy way to manage
 * devices that are compatible with implemented interface.
 * In the current state it's Smarwi windows maintainer.
 */
class VektivaDeviceManager : public DeviceManager {
public:

	/**
	 * @brief Provides searching of Vektiva devices on network via MQTT messages
	 * in an own thread.
	 */
	class VektivaSeeker : public AbstractSeeker {
	public:
		typedef Poco::SharedPtr<VektivaSeeker> Ptr;
		VektivaSeeker(VektivaDeviceManager &parent,
			const Poco::Timespan &duration);

	protected:

		/**
		 * Seeking for devices, processing and checking whether seek interval
		 * timed out.
		 * @param control
		 */
		void seekLoop(StopControl &) override;

	private:
		VektivaDeviceManager &m_parent;
	};

	VektivaDeviceManager();
	void run() override;
	void stop() override;
	void setMqttClient(MqttClient::Ptr mqttClient);
	void setStatusMqttClient(MqttClient::Ptr mqttClient);
	void setReceiveTimeout(const Poco::Timespan &timeout);

protected:
	AsyncWork<>::Ptr startDiscovery(const Poco::Timespan &timeout) override;
	AsyncWork<std::set<DeviceID>>::Ptr startUnpair(
		const DeviceID &id,
		const Poco::Timespan &) override;
	AsyncWork<double>::Ptr startSetValue(
		const DeviceID &id,
		const ModuleID &module,
		const double value,
		const Poco::Timespan &) override;
	void handleAccept(const DeviceAcceptCommand::Ptr cmd) override;

	/**
	 * @brief New device is being processed. If true is returned,
	 * NewDeviceCommand can be dispatched.
	 * @param newDevice the device to process
	 * @return true on success, false otherwise
	 */
	bool updateDevice(VektivaSmarwi::Ptr newDevice);

	/**
	 * @brief Finds a device with the corresponding device ID
	 * and attempts to change the state of the selected module ID.
	 * @param deviceID ID of the device to be modified
	 * @param moduleID ID of the module to be modified
	 * @param value value to change to
	 * @throws InvalidArgumentException thrown when no device is found
	 */
	void modifyValue(
		const DeviceID &deviceID,
		const ModuleID &moduleID,
		const double value);

	/**
	 * @brief Groups all actions that are done
	 * when "status" message is received.
	 * @param mqttMessage received by MQTT client
	 */
	void statusMessageAction(MqttMessage &mqttMessage);

	/**
	 * @brief Groups all actions that are done
	 * when "online" message is received.
	 * @param mqttMessage received by MQTT client
	 */
	void onlineMessageAction(MqttMessage &mqttMessage);

	/**
	 * @brief Clears all messages buffered in the MQTT client to assure
	 * there are no previous messages when attempting to contact a device.
	 */
	void clearMqttMessageBuffer();

	/**
	 * @brief Parses the received message and according to its' content,
	 * correct actions are performed.
	 * @param message a message received by the MQTT client
	 */
	void analyzeMessage(MqttMessage &mqttMessage);

	/**
	 * @brief Waits for a specified amount of time for a message
	 * with the last segment of the topic equal to lastSegment argument
	 * and device properties.
	 * @param timeout how long to wait for the status message
	 * @param lastSegment in the topic
	 * @param device Vektiva device
	 * @throws TimeoutException if timeout occured
	 * @return MqttMessage
	 */
	MqttMessage messageReceivedInTime(
		const std::string &lastSegment,
		VektivaSmarwi::Ptr device);

	/**
	 * @brief Sends Smarwi status to the exporters.
	 * @param deviceId device ID of the device that has changed status
	 * @param smarwiStatus status of Smarwi
	 */
	void shipSmarwiStatus(
		const DeviceID &deviceId,
		const VektivaSmarwiStatus &smarwiStatus);

	/**
	 * @brief Dispatches NewDeviceCommand.
	 * @param device device to dispatch
	 */
	void dispatchNewDevice(VektivaSmarwi::Ptr device);

	/**
	 * @brief When status message is updated, info is parsed and device pointer
	 * passed in argument is updated with info from the message.
	 * @param timeout how long to wait for the status message
	 * @param device Vektiva device
	 * @return true if status message arrived, false otherwise
	 */
	bool receiveStatusMessageAndUpdate(
		VektivaSmarwi::Ptr device);

	/**
	 * @brief Function to update any relevant info
	 * that can be updated (e.g. IP address)
	 * @param device device to update
	 * @param smarwiStatus update with this data
	 */
	void updateDeviceInfo(
		VektivaSmarwi::Ptr device,
		const VektivaSmarwiStatus &smarwiStatus);

	/**
	 * @brief Function to validate topic in received message
	 * Last two parameters are optional and can either specify the exact
	 * Remote ID and MAC address or if left blank, they'll check for rules
	 * that topic have to have.
	 *
	 * Regex explained
	 * ^ - regex from the beginning
	 * ion/ - basic prefix for Smarwi
	 * [^#\/]+ OR <REMOTEID - can input anything as RemoteID except # and / OR
	 * topic has to be equal RemoteId specified as the parameter
	 * /% - delimiter
	 * [a-fA-F0-9]{12} OR <MACADDR> - any MAC address OR exact MAC address
	 * specified in the parameter
	 * /<LASTSEGMENT> - type of the message e.g. status / online / cmd
	 * $ - until the end
	 *
	 * Examples:
	 * isTopicValid(
	 * "ion/dowarogxby/%aabbccaabbcc/online",
	 * "online",
	 * "dowarogxby",
	 * "aabbccaabbcc")
	 * returns true because every segment of the topic is
	 * equal to the corresponding segments.
	 *
	 * isTopicValid(
	 * "ion/dowarogxby/%aabbccaabbcc/status",
	 * "online")
	 * returns false because the last segment doesn't match
	 * @param topic topic of received message
	 * @param lastSegment is what should topic contain as the last word
	 * @param remoteId remoteId that should topic be equal to
	 * @param macAddr MAC address that should topic be equal to
	 * @return true if valid, false otherwise
	 */
	bool isTopicValid(
		const std::string &topic,
		const std::string &lastSegment,
		const std::string &remoteId = "",
		const std::string &macAddr = "");

	/**
	 * @brief Exctracts Remote ID and MAC address from MQTT message' topic.
	 * @param topic topic of received message
	 * @return std::pair<std::string, std::string> First is a Remote ID,
	 * the second is a MAC address.
	 */
	std::pair<std::string, std::string>
	retrieveDeviceInfoFromTopic(const std::string &topic);

	/**
	 * @brief Escapes inputted string from regex control characters
	 * @param regexString string to be escaped
	 */
	void escapeRegexString(std::string &regexString);

private:
	/**
	 * @brief Mutex for shared access to m_devices.
	 */
	Poco::FastMutex m_devicesMutex;

	/**
	 * @brief Mutex for shared access to the MQTT client for manipulation
	 * with Smarwis.
	 */
	Poco::FastMutex m_clientMqttMutex;

	/**
	 * @brief List of found devices.
	 */
	std::map<DeviceID, VektivaSmarwi::Ptr> m_devices;

	/**
	 * @brief MQTT client instance to manipulate with Smarwis.
	 */
	MqttClient::Ptr m_mqttClient;

	/**
	 * @brief MQTT client instance which sole purpose is to receive messages
	 * and analyze them. It is only used in the main loop and should not be
	 * used anywhere else.
	 */
	MqttClient::Ptr m_mqttStatusClient;
	Poco::Timespan m_receiveTimeout;
};
}
