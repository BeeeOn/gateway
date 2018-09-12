#pragma once

#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timer.h>
#include <Poco/Timestamp.h>

#include <fitp/fitp.h>

#include "core/DeviceManager.h"
#include "core/GatewayInfo.h"
#include "fitp/FitpDevice.h"
#include "model/DeviceID.h"

namespace BeeeOn {

/**
 * Ensures initialization of FIT protocol (fitp). It is able to send
 * NewDeviceCommand to CommandDispatcher when device attempts to pair.
 *
 * It also ensures reaction to commands sent from server:
 *
 * - GatewayListenCommand, DeviceAcceptCommand - device attempts to pair
 * - DeviceUnpairCommand - device attempts to unpair
 *
 * It ships measured data to Distributor.
 *
 * It processes DATA and JOIN REQUEST messages.
 *
 * Generally, message has the following format:
 * - message type [1 B] - DATA or JOIN REQUEST message
 * - device type [1 B] - end device or coordinator
 * - edid [4 B] - end device identifier
 * - data [x B] - data on application layer
 *
 * Format of application data:
 * - command [1 B] - FROM_SENSOR_MSG
 * - version [1 B] - application protocol version
 * - device ID [2 B] - device identifier
 * - pairs [1 B] - count of pairs: module ID, value
 * - module ID [1 B] - module identifier
 * - value [2 B/4 B] - value of module, length of value depends on module type
 * -- 2 B - battery
 * -- 4 B - inner/outer temperature, humidity
 *
 * Note: RSSI is obtained in FitpDeviceManager, value has 1 B.
 *
 * Note: Only one instance of FitpDeviceManager is possible, because
 * fitp library does not support more instances.
 */
class FitpDeviceManager : public DeviceManager {
public:
	typedef Poco::SharedPtr<FitpDeviceManager> Ptr;
	typedef uint32_t EDID;

	FitpDeviceManager();
	~FitpDeviceManager();

	void run() override;
	void stop() override;

	/**
	 * Sets devices as available and paired according to the device list sent from server
	 * and the map of devices. A device has to be in the map of devices
	 * so that it can be loaded from server.
	 */
	void loadDeviceList();

	/**
	 * Sets path to the configuration file from which device table on PAN coordinator is filled.
	 * If device table is not filled, PAN coordinator is not able to react to the DATA messages from
	 * a device. If a device wants to join to the network, it sends the JOIN REQUEST message that
	 * PAN coordinator is able to receive and process. After successful joining process, this device will
	 * be stored into the device table and saved to the file.
	 */
	void setConfigPath(const std::string &configPath);

	/**
	 * Sets minimum value of noise threshold.
	 * It is necessary to set so that data can be sent.
	 * Condition: min >= 0
	 */
	void setNoiseMin(int min);

	/**
	 * Sets maximum value of noise threshold.
	 * It is necessary to set so that data can be sent.
	 * Condition: max >= 0
	 */
	void setNoiseMax(int max);

	/**
	 * Sets bitrate.
	 * It is possible to change bitrate.
	 * 0x00 - DATA_RATE_5
	 * 0x01 - DATA_RATE_10
	 * 0x02 - DATA_RATE_20
	 * 0x03 - DATA_RATE_40
	 * 0x04 - DATA_RATE_50
	 * 0x05 - DATA_RATE_66
	 * 0x06 - DATA_RATE_100
	 * 0x07 - DATA_RATE_200
	 */
	void setBitrate(int bitrate);


	/**
	 * Sets band.
	 * It is possible to change band.
	 * Following values are supported:
	 * - 0 - band 863-870 MHz
	 * - 1 - band 950-960 MHz
	 * - 2 - band 902-915 MHz
	 * - 3 - band 915-928 MHz
	 */
	void setBand(int band);

	/**
	 * Sets channel.
	 * It is possible to change channel.
	 * Case 1: (band = 0 or band = 1) and (bitrate = 6 or bitrate = 7)
	 *         values 0-24 are valid
	 * Case 2: otherwise
	 *         values 0-31 are valid
	 *
	 * Note: Band and bitrate have to be set before setting a channel.
	 * It is used during joining process when channel scanning is performed.
	 */
	void setChannel(int channel);

	/**
	 * Sets transmission power.
	 * It is possible to change TX power.
	 * The TX power specifies the strength of the signal that device produces during
	 * data transmitting.
	 * Following values are supported:
	 * 0x00: 13 dBm
	 * 0x01: 10 dBm
	 * 0x02: 7 dBm
	 * 0x03: 4 dBm
	 * 0x04: 1 dBm
	 * 0x05: -2 dBm
	 * 0x06: -5 dBm
	 * 0x07: -8 dBm
	 *
	 * Note: If TX power is set to 7 dBm, device is able to communicate with near devices.
	 * If TX power is set to 13 dBm, device is able to communicate with more distant devices.
	 */
	void setPower(int power);

	/**
	 * Sets count of attempts to resend a packet.
	 * Packet can be resent if devices communicate using four-way handshake.
	 * Condition: retries >= 0
	 */
	void setTxRetries(int retries);

	void setGatewayInfo(Poco::SharedPtr<GatewayInfo> info);

	/**
	 * Initializes the fitp.
	 */
	void initFitp();

	/**
	* Converts end device identifier (EDID) to device ID.
	* EDID is array of four uint8_t values.
	* EDID is stored in lower 32 bits of DeviceID from the index 0
	* to index 4.
	 * Example:
	 * 0 0 0 0 edid[0] edid[1] edid[2] edid[3]
	*/
	static DeviceID buildID(EDID edid);

	/**
	* Converts device ID to end device identifier (EDID).
	*/
	static EDID deriveEDID(const DeviceID &id);

	/**
	* Parses EDID from received data.
	*/
	EDID parseEDID(const std::vector<uint8_t> &id);

	/**
	* Processes data message sent by device.
	*/
	void processDataMsg(const std::vector<uint8_t> &data);

	/**
	* Processes join request message sent by device.
	*/
	void processJoinMsg(const std::vector<uint8_t> &data);

protected:
	void handleGeneric(const Command::Ptr cmd, Result::Ptr result) override;

	/**
	 * Reacts to ListenCommand. It sends NewDeviceCommand if
	 * device is not paired or is not available.
	 */
	void doListenCommand(const GatewayListenCommand::Ptr cmd);

	/**
	 * Reacts to AcceptCommand. Device has to be stored in map
	 * of devices, it has to be available and it has to be unpaired.
	 * If these conditions are fulfilled, method inserts device into a map of devices,
	 * and it sets device as paired.
	 */
	void doDeviceAcceptCommand(const DeviceAcceptCommand::Ptr cmd);

	/**
	* Reacts to UnpairCommand. Device has to be in map of
	* devices, it has to be paired and available.
	*/
	void doUnpairCommand(const DeviceUnpairCommand::Ptr cmd);

	/**
	 * Ensures sending of NewDeviceCommand to CommandDispatcher.
	 */
	void dispatchNewDevice(FitpDevice::Ptr device);

private:
	void stopListen(Poco::Timer &timer);
	int channelCnt();

private:
	std::map<DeviceID, FitpDevice::Ptr> m_devices;
	std::string m_configFile;
	PHY_init_t m_phyParams;
	LINK_init_t m_linkParams;
	bool m_listening;
	Poco::TimerCallback<FitpDeviceManager> m_listenCallback;
	Poco::Timer m_listenTimer;
	Poco::FastMutex m_lock;
	Poco::SharedPtr<GatewayInfo> m_gatewayInfo;
};

}
