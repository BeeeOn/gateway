#pragma once

#include "core/Command.h"
#include "model/DeviceID.h"
#include "model/ModuleID.h"

namespace BeeeOn {

/*
 * Server finds out value from entered DeviceID and ModuleID,
 * which was last set/sent. The last value saved in database
 * is the result of this command.
 */
class ServerLastValueCommand : public Command {
public:
	typedef Poco::AutoPtr<ServerLastValueCommand> Ptr;

	ServerLastValueCommand(const DeviceID &deviceID,
		const ModuleID &moduleID);

	DeviceID deviceID() const;
	ModuleID moduleID() const;

	std::string toString() const override;

protected:
	~ServerLastValueCommand();

private:
	DeviceID m_deviceID;
	ModuleID m_moduleID;
};

}
