#pragma once

#include <list>
#include <map>
#include <string>
#include <vector>

#include <Poco/JSON/Object.h>
#include <Poco/Mutex.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>

#include "model/DeviceDescription.h"
#include "model/DeviceID.h"
#include "model/GatewayID.h"
#include "model/ModuleID.h"
#include "model/ModuleType.h"
#include "model/RefreshTime.h"
#include "model/SensorData.h"
#include "net/HTTPEntireResponse.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief The class represents Thermona Regulator VPT LAN v1.0.
 * It provides functions to control the VPT and to gather data from
 * its sensors. Each VPT regulator consists of up to 4 zones and 1 boiler.
 * This means one instance of VPTDevice takes care of 5 devices. Each subdevice
 * has own DeviceID.
 */
class VPTDevice : protected Loggable {
public:
	typedef Poco::SharedPtr<VPTDevice> Ptr;

	static const std::vector<std::string> REG_BOILER_OPER_TYPE;
	static const std::vector<std::string> REG_BOILER_OPER_MODE;
	static const std::vector<std::string> REG_MAN_ROOM_TEMP;
	static const std::vector<std::string> REG_MAN_WATER_TEMP;
	static const std::vector<std::string> REG_MAN_TUV_TEMP;
	static const std::vector<std::string> REG_MOD_WATER_TEMP;

	static const std::list<ModuleType> ZONE_MODULE_TYPES;
	static const std::list<ModuleType> BOILER_MODULE_TYPES;

	static const int COUNT_OF_ZONES;

	enum Action {
		PAIR = 0x01,
		READ,
		SET
	};

	/**
	 * @brief Connects to specified address to fetch information for creating VPT Device.
	 * If the device do not respond in specified timeout, Poco::TimeoutException is thrown.
	 * @param &address IP address and port where the device is listening.
	 * @param httpTimeout HTTP timeout.
	 * @param pingTimeout ping timeout used to obtain the IP address of gateway's interface
	 * from which the VPT is available.
	 * @param id Gateway id used in generating of stamp.
	 */
	VPTDevice(
		const Poco::Net::SocketAddress& address,
		const Poco::Timespan& httpTimeout,
		const Poco::Timespan& pingTimeout,
		const GatewayID& id);

	DeviceID boilerID() const;
	Poco::Net::SocketAddress address() const;
	void setAddress(const Poco::Net::SocketAddress& address);
	void setPassword(const std::string& pwd);
	Poco::FastMutex& lock();

	/**
	 * @brief Creates a stamp that consists of gateway id, IP address of gateway's interface
	 * from whitch the VPT is available, time, and action.
	 */
	std::string generateStamp(const Action action);

	/**
	 * @brief Sends stamp with aciton to the VPT. It serves to verify that
	 * the gateway communicates with the VPT.
	 */
	void stampVPT(const Action action);

	/**
	 * @param id Subdevice of VPT.
	 * @param module Module of subdevice.
	 * @param value Requested value.
	 * the success of the request
	 */
	void requestModifyState(const DeviceID& id, const ModuleID module,
		const double value);

	/**
	 * @brief Gathers data from all sensors of subdevices.
	 * @return List of SensorData.
	 */
	std::vector<SensorData> requestValues();

	/**
	 * @brief Returns list of new device commands of all subdevices.
	 */
	std::vector<DeviceDescription> descriptions(const RefreshTime& refresh) const;

	/**
	 * @brief Compares two VPTs based on DeviceID.
	 */
	bool operator==(const VPTDevice& other) const;

	/**
	 * @brief Creates DeviceID from VPT DeviceID and
	 * number of zone.
	 */
	static DeviceID createSubdeviceID(const int zone, const DeviceID& id);

	/**
	 * @brief Returns DeviceID of real VPT.
	 */
	static DeviceID omitSubdeviceFromDeviceID(const DeviceID& id);

	/**
	 * @brief Extracts subdevice number from DeviceID.
	 */
	static int extractSubdeviceFromDeviceID(const DeviceID& id);

	static std::string extractNonce(const std::string& response);
	static std::string generateHashPassword(const std::string& pwd, const std::string& response);

private:
	/**
	 * @brief Creates DeviceID based on Mac address of device.
	 */
	void buildDeviceID();

	void requestSetModBoilerOperationType(const int zone, const double value);
	void requestSetModBoilerOperationMode(const int zone, const double value);
	void requestSetManualRoomTemperature(const int zone, const double value);
	void requestSetManualWaterTemperature(const int zone, const double value);
	void requestSetManualTUVTemperature(const int zone, const double value);
	void requestSetModWaterTemperature(const int zone, const double value);

	std::string parseZoneAttrFromJson(const std::string& json, const int zone, const std::string& key);

	HTTPEntireResponse prepareAndSendRequest(const std::string& registr, const std::string& value);

	/**
	 * @brief It sends the set HTTP request. If the password is required
	 * it sends request with set password.
	 * @param request HTTP request.
	 * @return HTTP response.
	 */
	HTTPEntireResponse sendSetRequest(Poco::Net::HTTPRequest& request);
	HTTPEntireResponse sendRequest(Poco::Net::HTTPRequest& request, const Poco::Timespan& timeout) const;

private:
	/**
	 * DeviceIDs of subdevices are created by this DeviceID.
	 * Also it is used to search a password to access in credentials storage.
	 */
	DeviceID m_boilerId;
	Poco::Net::SocketAddress m_address;
	std::string m_password;

	Poco::Timespan m_pingTimeout;
	Poco::Timespan m_httpTimeout;

	GatewayID m_gatewayID;
	Poco::FastMutex m_lock;
};

}
