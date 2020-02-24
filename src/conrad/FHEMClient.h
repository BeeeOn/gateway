#pragma once

#include <map>
#include <queue>
#include <string>
#include <vector>

#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/Timestamp.h>
#include <Poco/Net/DialogSocket.h>
#include <Poco/Net/SocketAddress.h>

#include "conrad/FHEMDeviceInfo.h"
#include "loop/StopControl.h"
#include "loop/StoppableRunnable.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief The class communicates with FHEM server. It allows
 * to search HomeMatic devices, gather data from HomeMatic devices
 * and send commands to change state of a device.
 */
class FHEMClient: Loggable, public StoppableRunnable {
public:
	typedef Poco::SharedPtr<FHEMClient> Ptr;

	FHEMClient();

	void setRefreshTime(const Poco::Timespan &time);
	void setReceiveTimeout(const Poco::Timespan &timeout);
	void setReconnectTime(const Poco::Timespan &time);
	void setFHEMAddress(const std::string &address);

	/**
	 * @brief It starts with creation of telnet connection with
	 * FHEM server and then it periodicly ask FHEM server about
	 * new events.
	 */
	void run() override;
	void stop() override;

	/**
	 * @brief Sends a request over a telnet connection to FHEM server.
	 */
	void sendRequest(const std::string& request);

	/**
	 * @brief Waiting for a new event according to given timeout.
	 * Returns event, if some event is in queue, otherwise
	 * waiting for a new event or timeout exception.
	 *
	 * This method should not be called by multiple threads,
	 * received message could be given to only one thread.
	 *
	 * Timeout:
	 *  - 0 - non-blocking
	 *  - negative - blocking
	 *  - positive blocking with timeout
	 */
	Poco::JSON::Object::Ptr receive(const Poco::Timespan &timeout);

protected:
	/**
	 * @brief Creates DialogSocket and connect it to a defined
	 * socket address m_fhemAddress.
	 */
	void initConnection();

	/**
	 * @brief Retrieves all HomeMatic devices known to FHEM server and
	 * processes each device. Processing of device The processing of
	 * a device consists of a detection and creation of an event.
	 */
	void cycle();

	/**
	 * @brief Returns message from queue. If the queue is empty
	 * it returns null.
	 */
	Poco::JSON::Object::Ptr nextEvent();

	/**
	 * @brief Retrieves all HomeMatic devices known to FHEM server.
	 */
	void retrieveHomeMaticDevices(std::vector<std::string>& devices);

	/**
	 * @brief Retrieves information about given device and detects
	 * changes connected to this device. If some change is detected then
	 * an event is created and is appended to the queue.
	 *
	 * Events: new_device, message, rcv_cnt, snd_cnt
	 */
	void processDevice(const std::string& device);

	/**
	 * @brief Creates DeviceInfo for a given device and
	 * part of message.
	 */
	FHEMDeviceInfo assembleDeviceInfo(
		const std::string& device,
		const Poco::JSON::Object::Ptr internals) const;

	/**
	 * @brief For a given device it retrieves all channels and their states.
	 */
	void retrieveChannelsState(
		Poco::JSON::Object::Ptr internals,
		std::map<std::string, std::string>& channels);

	/**
	 * @brief Retrieves a state of a given channel.
	 */
	std::string retrieveChannelState(const std::string& channel);

	void createNewDeviceEvent(
		const std::string& device,
		const std::string& model,
		const std::string& type,
		const std::string& serialNumber);

	void createStatEvent(
		const std::string& event,
		const std::string& device);

	void createMessageEvent(
		const std::string& device,
		const std::string& model,
		const std::string& type,
		const std::string& serialNumber,
		const std::string& rawMsg,
		const double rssi,
		const std::map<std::string, std::string>& channels);

	void appendEventToQueue(const Poco::JSON::Object::Ptr event);

	/**
	 * @brief Sends a command over telnet connection and returns a response.
	 *
	 * @throws TimeoutException in case of expiration of receive timeout.
	 * @throws SyntaxException in case of wrong response.
	 */
	Poco::JSON::Object::Ptr sendCommand(const std::string& command);

private:
	StopControl m_stopControl;
	Poco::Timespan m_refreshTime;
	Poco::Timespan m_receiveTimeout;
	Poco::Timespan m_reconnectTime;
	Poco::Net::SocketAddress m_fhemAddress;
	Poco::Net::DialogSocket m_telnetSocket;
	std::map<std::string, FHEMDeviceInfo> m_deviceInfos;
	std::queue<Poco::JSON::Object::Ptr> m_eventsQueue;
	Poco::FastMutex m_queueMutex;
	Poco::Event m_receiveEvent;
};

}
