#pragma once

#include <list>
#include <string>

#include <Poco/SharedPtr.h>

#include "model/ModuleType.h"
#include "model/DeviceID.h"
#include "model/SensorData.h"
#include "util/Loggable.h"

namespace BeeeOn {

class JablotronDevice : public Loggable {
public:
	typedef Poco::SharedPtr<JablotronDevice> Ptr;

	virtual ~JablotronDevice();

	void setPaired(bool paired);
	bool paired() const;

	DeviceID deviceID() const;

	static DeviceID buildID(uint32_t serialNumber);
	static JablotronDevice::Ptr create(uint32_t serialNumber);

	/**
	 * It should extract the device-specific values and DeviceID from
	 * Jablotron message and creates SensorData.
	 * Example of Jablotron message: [XXXXXXXX] JA-81M SENSOR LB:? ACT:1
	 *
	 * See: https://www.turris.cz/gadgets/manual
	 */
	virtual SensorData extractSensorData(const std::string &message) = 0;

	/**
	 * List of supported value types with attributes.
	 */
	virtual std::list<ModuleType> moduleTypes() = 0;

	/**
	 * Name of Jablotron device.
	 */
	std::string name() const;

	virtual Poco::Timespan refreshTime();

protected:
	JablotronDevice(const DeviceID &deviceID, const std::string &name);

	/**
	 * It divides a string consisting of two parts. Delimiter of these
	 * two parts is colon. The first part contains value type and
	 * the second part contains value.
	 * Example: ACT:0 or LB:0
	 */
	int parseValue(const std::string &msg) const;

	/**
	 * Extracts battery level from string. String contains two parts.
	 * The first part is "LB" (value type) and the second part is
	 * battery level. Delimiter of these two parts is colon.
	 * Battery level is 0 or 1. 0 doesn't indicate low battery level (100%),
	 * and 1 indicates low battery level (5%).
	 * Example of input string: LB:0 or LB:1
	 *
	 * See: https://www.turris.cz/gadgets/manual
	 */
	int extractBatteryLevel(const std::string &battery);

protected:
	static const Poco::Timespan REFRESH_TIME_SUPPORTED_BEACON;
	static const Poco::Timespan REFRESH_TIME_UNSUPPORTED_BEACON;

private:
	DeviceID m_deviceID;
	bool m_paired;
	std::string m_name;
};

}
