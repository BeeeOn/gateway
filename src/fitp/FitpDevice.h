#pragma once

#include <Poco/SharedPtr.h>

#include "core/DeviceManager.h"
#include "model/DeviceID.h"
#include "model/ModuleType.h"
#include "model/SensorData.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * Keeps information about devices that are paired within the BeeeOn system.
 * If device has set paired flag to true, it is paired in the BeeeOn system.
 */
class FitpDevice : public Loggable {
public:
	enum DeviceType {
		// end device - it can send values and it is not able to route data
		END_DEVICE,
		// coordinator - it can send values and it is able to route data
		COORDINATOR,
	};
	typedef Poco::SharedPtr<FitpDevice> Ptr;

	FitpDevice(DeviceID id, bool paired = false);
	virtual ~FitpDevice();

	DeviceID deviceID() const;
	void setDeviceID(const DeviceID &deviceId);

	DeviceType type() const;

	/**
	 * Returns module types of device.
	 * ED - battery, inner temperature, outer temperature, humidity, RSSI module.
	 * COORD - battery, inner temperature, RSSI module.
	 */
	std::list<ModuleType> modules(const DeviceType &type);

	/**
	 * Returns count of bytes that has to be skipped in case of end device.
	 */
	size_t moduleEDOffset(const uint8_t &id);

	/**
	 * Returns count of bytes that has to be skipped in case of coordinator.
	 */
	size_t moduleCOORDOffset(const uint8_t &id);

	/**
	 * Processes data received from device. It is able to recognize a particular
	 * module and its value.
	 */
	SensorData parseMessage(const std::vector<uint8_t> &data, DeviceID deviceID);

	/**
	 * Returns module value.
	 */
	static double extractValue(const std::vector<uint8_t> &values);
	static double voltsToPercentage(double milivolts);

	/**
	 * Returns modified module value.
	 */
	static double moduleValue(uint8_t id, const std::vector<uint8_t> &data);

	/**
	 * Returns end device (ED) module ID.
	 */
	static ModuleID deriveEDModuleID(const uint8_t id);

	/**
	 * Returns coordinator (COORD) module ID.
	 */
	static ModuleID deriveCOORDModuleID(const uint8_t id);

	void setPaired(bool paired);
	bool paired();
private:
	DeviceID m_deviceID;
	DeviceType m_type;
	bool m_paired;
};

}
