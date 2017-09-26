#ifndef BEEEON_DEVICE_ACCEPTED_COMMAND_H
#define BEEEON_DEVICE_ACCEPTED_COMMAND_H

#include "core/Command.h"
#include "model/DeviceID.h"

namespace BeeeOn {

/*
 * The server informs gateway about accepting of a new
 * device from user.
 */
class DeviceAcceptCommand : public Command {
public:
	typedef Poco::AutoPtr<DeviceAcceptCommand> Ptr;

	DeviceAcceptCommand(const DeviceID &deviceID);

	DeviceID deviceID() const;

	std::string toString() const override;

protected:
	~DeviceAcceptCommand();

private:
	DeviceID m_deviceID;
};

}

#endif
