#ifndef BEEEON_DEVICE_UNPAIR_COMMAND_H
#define BEEEON_DEVICE_UNPAIR_COMMAND_H

#include "core/Command.h"
#include "model/DeviceID.h"

namespace BeeeOn {

/*
 *  The device manager receives the requirement and it
 *  deletes this device from its network. The device
 *  manager receives the requirement and it deletes
 *  this device from its network. The individual states
 *  of the unpaired to which the command can get need
 *  to be reported on server.
 */
class DeviceUnpairCommand : public Command {
public:
	typedef Poco::AutoPtr<DeviceUnpairCommand> Ptr;

	DeviceUnpairCommand(const DeviceID &deviceID);

	DeviceID deviceID() const;

	std::string toString() const override;

protected:
	~DeviceUnpairCommand();

private:
	DeviceID m_deviceID;
};

}

#endif
