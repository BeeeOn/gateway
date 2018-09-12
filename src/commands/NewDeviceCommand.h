#pragma once

#include <list>

#include <Poco/Timespan.h>

#include "core/Command.h"
#include "model/DeviceID.h"
#include "model/ModuleType.h"

namespace BeeeOn {

/*
 * The command that informs the server about new device and
 * data types, which it sends.
 */
class NewDeviceCommand : public Command {
public:
	typedef Poco::AutoPtr<NewDeviceCommand> Ptr;

	NewDeviceCommand(const DeviceID &deviceID, const std::string &vendor,
		const std::string &productName, const std::list<ModuleType> &dataTypes,
		Poco::Timespan refreshTime = -1);

	DeviceID deviceID() const;
	std::string vendor() const;
	std::string productName() const;
	std::list<ModuleType> dataTypes() const;
	bool supportsRefreshTime() const;
	Poco::Timespan refreshTime() const;

	std::string toString() const override;

protected:
	~NewDeviceCommand();

private:
	DeviceID m_deviceID;
	std::string m_vendor;
	std::string m_productName;
	std::list<ModuleType> m_dataTypes;
	Poco::Timespan m_refreshTime;
};

}
