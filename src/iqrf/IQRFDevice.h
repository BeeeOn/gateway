#pragma once

#include <list>
#include <string>

#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>

#include "core/PollableDevice.h"
#include "iqrf/DPAMessage.h"
#include "iqrf/DPAProtocol.h"
#include "iqrf/IQRFEventFirer.h"
#include "iqrf/IQRFMqttConnector.h"
#include "model/DeviceID.h"
#include "model/ModuleType.h"
#include "model/RefreshTime.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief IQRFDevice represents information about a particular device from
 * the IQRF network. Each IQRF device is identified by network address.
 * Network address is a unique identifier in IQRF network. Each IQRF device
 * has IQRF transceiver. The MID (Module ID) is globally unique. Each IQRF
 * device can communicate using own or general protocol.
 *
 * The class allows to get basic device information such as Mid, supported
 * modules, HWPID, and peripheral info.
 */
class IQRFDevice : public PollableDevice, Loggable {
public:
	typedef Poco::SharedPtr<IQRFDevice> Ptr;

	IQRFDevice(
		IQRFMqttConnector::Ptr connector,
		const Poco::Timespan &receiveTimeout,
		DPAMessage::NetworkAddress address,
		DPAProtocol::Ptr protocol,
		const RefreshTime &refreshTime,
		const RefreshTime &refreshTimePeripheralInfo,
		IQRFEventFirer::Ptr eventFirer);

	/**
	 * @returns identification of node in the IQRF network.
	 */
	DPAMessage::NetworkAddress networkAddress() const;

	/**
	 * @returns mid unique identification of IQRF transceiver.
	 */
	uint32_t mid() const;

	/**
	 * @brief Protocol that communicates with the coordinator.
	 */
	DPAProtocol::Ptr protocol() const;

	/**
	 * @brief Supported modules on the device.
	 *
	 * @returns empty list if device can not obtain modules from device.
	 */
	std::list<ModuleType> modules() const;

	/**
	 * @brief Identification of node in the IQRF repository. Device
	 * name and manufacturer can be obtained on the basis of the number.
	 */
	uint16_t HWPID() const;
	void setHWPID(uint16_t HWPID);

	/**
	 * @return Creates deviceID from:
	 *
	 *  - Device Prefix (1B)
	 *  - Zero byte (1B)
	 *  - IQRF MID (4B)
	 *  - HWPID (2B)
	 */
	DeviceID id() const override;

	std::string vendorName() const;
	std::string productName() const;

	RefreshTime refresh() const override;
	void poll(Distributor::Ptr distributor) override;


	/**
	* @brief Converts all device parameters into one string for
	* easy viewing.
	*/
	std::string toString() const;

	/**
	 * @brief Probes information about IQRF device in a network.
	 */
	void probe(const Poco::Timespan &methodTimeout);

	/**
	 * @return SensorData from values measured by the sensor.
	 */
	SensorData obtainValues();

	/**
	 * @return SensorData from peripheral info (battery and RSSI)
	 * of the sensor.
	 */
	SensorData obtainPeripheralInfo();

private:
	/**
	 * @return Identification that uniquely specifies the functionality of the
	 * device, the user peripherals it implements, its behavior etc.
	 */
	uint16_t detectNodeHWPID(const Poco::Timespan &methodTimeout);

	/**
	 * @brief Detects global unique identification of IQRF device.
	 *
	 * @return Global unique identification of IQRF device.
	 */
	uint32_t detectMID(const Poco::Timespan &methodTimeout);

	/**
	 * @brief Detects supported modules used by IQRF device.
	 *
	 * @return Supported modules used by IQRF device.
	 */
	std::list<ModuleType> detectModules(
		const Poco::Timespan &methodTimeout);

	/**
	 * @brief Detects product name and vendor of IQRF device.
	 *
	 * @return Product name and vendor of IQRF device.
	 */
	DPAProtocol::ProductInfo detectProductInfo(
		const Poco::Timespan &methodTimeout);

	ModuleID batteryModuleID() const;
	ModuleID rssiModuleID() const;

private:
	IQRFMqttConnector::Ptr m_connector;
	Poco::Timespan m_receiveTimeout;
	DPAMessage::NetworkAddress m_address;
	DPAProtocol::Ptr m_protocol;
	RefreshTime m_refreshTime;
	RefreshTime m_refreshTimePeripheralInfo;

	mutable Poco::Timespan m_remainingValueTime;
	mutable Poco::Timespan m_remainingPeripheralInfoTime;
	mutable Poco::Timespan m_remaining;

	uint32_t m_mid;
	std::list<ModuleType> m_modules;
	uint16_t m_HWPID;
	std::string m_vendorName;
	std::string m_productName;

	IQRFEventFirer::Ptr m_eventFirer;
};

}
