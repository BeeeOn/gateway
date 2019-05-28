#pragma once

#include <list>

#include "model/DeviceID.h"
#include "model/ModuleID.h"
#include "model/ModuleType.h"
#include "net/MqttClient.h"
#include "net/MACAddress.h"
#include "util/Loggable.h"
#include "vektiva/VektivaSmarwiStatus.h"

namespace BeeeOn {

/**
 * @brief The class represents a standalone device Smarwi.
 * It allows to communicate with the actual device via MQTT client and
 * thus control it.
 */
class VektivaSmarwi : protected Loggable {
public:
	typedef Poco::SharedPtr<VektivaSmarwi> Ptr;

	VektivaSmarwi(
		const MACAddress& macAddr,
		const std::string &remoteID);

	std::string remoteID();
	MACAddress macAddress();
	DeviceID deviceID();
	std::list<ModuleType> moduleTypes() const;
	Poco::Net::IPAddress ipAddress();
	void setIpAddress(const Poco::Net::IPAddress &ipAddress);
	std::string productName();

	/**
	 * @brief Attempts to change status of the device.
	 * @param moduleID - what module to change
	 * @param value - to what value to change to
	 * @param mqttClient - which MQTT client to use
	 */
	void requestModifyState(
		const ModuleID &moduleID,
		double value,
		MqttClient::Ptr mqttClient);

	/**
	 * @brief Creates Smarwi sensor data to send to registered exporters.
	 * @param smarwiStatus - status of Smarwi status
	 * @return SensorData - sensor data created out of SmarWiStatus
	 */
	SensorData createSensorData(const VektivaSmarwiStatus &smarwiStatus);

	/**
	 * @brief Called internally when constructing the instance.
	 * Creates DeviceID based on Mac address of device.
	 * @return DeviceID - DeviceID created out of MAC address provided
	 */
	static DeviceID buildDeviceID(
		const MACAddress &macAddrStr);

	/**
	 * @brief Constructs an MQTT message with the topic
	 * "ion/<remoteId>/%<macAddress>/cmd" and message specified
	 * in the command parameter.
	 * @param remoteId - remoteId of the user, can be found in Smarwi settings
	 * @param macAddress - mac address of Smarwi
	 * @param command - command to send to Smarwi
	 * @return MqttMessage on success
	 */
	static MqttMessage buildMqttMessage(
		const std::string &remoteId,
		const std::string &macAddress,
		const std::string &command);

	/**
	 * @brief Parses Smarwi's status response to an object which is returned
	 * if parsing is successful.
	 * @param rcvmsg MQTT message received from Smarwi after status request
	 * @return VektivaSmarwiStatus object representation of SmarWi
	 * status message
	 */
	static VektivaSmarwiStatus parseStatusResponse(
		std::string &rcvmsg);

protected:
	/**
	 * @brief Function checks if module id and it's value is valid and if so,
	 * publishes a command to change state of Smarwi.
	 * @param moduleID - ID of the module
	 * @param value - intended value
	 * @param mqttClient - client ptr which will publish the message
	 */
	void publishModifyStateCommand(
		const ModuleID &moduleID,
		double value,
		MqttClient::Ptr mqttClient);

	/**
	 * @brief After command to modify state was published, this function waits
	 * until the message with correct status is received.
	 * @param mqttClient - client ptr which will receive message
	 */
	void confirmStateModification(
		MqttClient::Ptr mqttClient);

	/**
	 * @breif Builds simple topic regex to validate the incoming message' topic.
	 * @return regex in string
	 */
	static std::string buildTopicRegex(
		const std::string &remoteId,
		const std::string &macAddress,
		const std::string &lastSegment);

private:
	DeviceID m_deviceId;
	std::string m_remoteID;
	MACAddress m_macAddress;
	Poco::Net::IPAddress m_ipAddress;
};
}
