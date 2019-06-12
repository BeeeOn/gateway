#pragma once

#include <list>
#include <string>

#include <Poco/SharedPtr.h>
#include <Poco/Timestamp.h>

#include "model/DeviceID.h"
#include "model/ModuleType.h"
#include "model/RefreshTime.h"
#include "model/SensorData.h"
#include "net/MqttMessage.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief Abstract class representing generic Sonoff device.
 */
class SonoffDevice : protected Loggable {
public:
	typedef Poco::SharedPtr<SonoffDevice> Ptr;

	static const std::string VENDOR_NAME;

	SonoffDevice(
		const DeviceID& id,
		const RefreshTime &refresh,
		const std::string& productName,
		const std::list<ModuleType>& moduleTypes);
	virtual ~SonoffDevice();

	DeviceID id() const;
	RefreshTime refreshTime() const;
	std::list<ModuleType> moduleTypes() const;
	std::string vendor() const;
	std::string productName() const;
	Poco::Timestamp lastSeen() const;

	/**
	 * @brief Transforms received MQTT message to SensorData.
	 */
	virtual SensorData parseMessage(const MqttMessage& message) = 0;

protected:
	const DeviceID m_deviceId;
	RefreshTime m_refresh;
	std::string m_productName;
	std::list<ModuleType> m_moduleTypes;
	Poco::Timestamp m_lastSeen;
};

}
