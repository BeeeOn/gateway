#pragma once

#include <Poco/Timespan.h>

#include "core/PrefixCommand.h"
#include "model/DeviceID.h"
#include "model/ModuleID.h"
#include "model/OpMode.h"

namespace BeeeOn {

/*
 * The command to set the value of a particular device.
 *
 * When system receives command, the device manager sets
 * the value of a device identified by DeviceID containing
 * the module identified by ModuleID. The value has to be
 * set before timeout expiration. If status of set value
 * cannot be find and the timeout is expired, device
 * manager must send message about this failure to server.
 * The individual states of the settings to which the command
 * can get need to be reported on server.
 *
 */
class DeviceSetValueCommand : public PrefixCommand {
public:
	typedef Poco::AutoPtr<DeviceSetValueCommand> Ptr;

	DeviceSetValueCommand(
		const DeviceID &deviceID,
		const ModuleID &moduleID,
		const double value,
		const OpMode &mode,
		const Poco::Timespan &timeout);

	ModuleID moduleID() const;
	double value() const;
	Poco::Timespan timeout() const;
	DeviceID deviceID() const;
	OpMode mode() const;

	std::string toString() const override;

protected:
	~DeviceSetValueCommand();

private:
	DeviceID m_deviceID;
	ModuleID m_moduleID;
	double m_value;
	OpMode m_mode;
	Poco::Timespan m_timeout;
};

}
