#pragma once

#include <Poco/Timespan.h>

#include "core/PrefixCommand.h"
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
class DeviceUnpairCommand : public PrefixCommand {
public:
	typedef Poco::AutoPtr<DeviceUnpairCommand> Ptr;

	DeviceUnpairCommand(
		const DeviceID &deviceID,
		const Poco::Timespan &timeout = 10 * Poco::Timespan::SECONDS);

	DeviceID deviceID() const;

	Poco::Timespan timeout() const;

	std::string toString() const override;

	Result::Ptr deriveResult(Answer::Ptr answer) const override;

protected:
	~DeviceUnpairCommand();

private:
	DeviceID m_deviceID;
	Poco::Timespan m_timeout;
};

}
