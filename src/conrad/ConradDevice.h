#pragma once

#include <list>
#include <string>

#include <Poco/SharedPtr.h>
#include <Poco/JSON/Object.h>

#include "model/DeviceID.h"
#include "model/ModuleType.h"
#include "model/RefreshTime.h"
#include "model/SensorData.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief Abstract class representing generic Conrad device.
 */
class ConradDevice : protected Loggable {
public:
	typedef Poco::SharedPtr<ConradDevice> Ptr;

	static const std::string VENDOR_NAME;

	ConradDevice(
		const DeviceID& id,
		const RefreshTime &refresh,
		const std::string& productName,
		const std::list<ModuleType>& moduleTypes);
	virtual ~ConradDevice();

	DeviceID id() const;
	RefreshTime refreshTime() const;
	std::list<ModuleType> moduleTypes() const;
	std::string vendor() const;
	std::string productName() const;

	/**
	 * @brief Transforms received ZMQ message to SensorData.
	 */
	virtual SensorData parseMessage(const Poco::JSON::Object::Ptr message) = 0;

	/**
	 * @brief Returns FHEM device id constructed from a given DeviceID.
	 */
	static std::string constructFHEMDeviceId(const DeviceID& id);

protected:
	/**
	 * @brief Returns true or false depending if the string is number or not.
	 */
	bool isNumber(const std::string& s);

protected:
	const DeviceID m_deviceId;
	RefreshTime m_refresh;
	std::string m_productName;
	std::list<ModuleType> m_moduleTypes;
};

}
