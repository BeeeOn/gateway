#pragma once

#include <list>
#include <string>

#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/UUID.h>

#include "bluetooth/BLESmartDevice.h"
#include "bluetooth/HciConnection.h"
#include "model/ModuleType.h"
#include "net/MACAddress.h"

namespace BeeeOn {

/**
 * @brief Abstract class for BeeWi devices. Some BeeWi devices need to set
 * the time to stop blinking. The method initLocalTime() is intended for it.
 */
class BeeWiDevice : public BLESmartDevice {
public:
	typedef Poco::SharedPtr<BeeWiDevice> Ptr;

private:
	/**
	 * UUID of characteristics containing actual time of the device.
	 */
	static const Poco::UUID LOCAL_TIME;
	static const std::string VENDOR_NAME;

public:
	BeeWiDevice(
		const MACAddress& address,
		const Poco::Timespan& timeout,
		const std::string& productName,
		const std::list<ModuleType>& moduleTypes);

	~BeeWiDevice();

	std::list<ModuleType> moduleTypes() const override;
	std::string vendor() const override;
	std::string productName() const override;

	void pair(
		HciInterface::Ptr hci,
		Poco::SharedPtr<HciInterface::WatchCallback> callback) override;

protected:
	/**
	 * @brief Sends and initilize local time of sensor. The local
	 * time is in format: %y%m%d%H%M%S.
	 *
	 * Exmaple: 5th December 2018 3:15:59 - 181205031559
	 */
	void initLocalTime(HciConnection::Ptr conn) const;

private:
	std::string m_productName;
	std::list<ModuleType> m_moduleTypes;
};

}
