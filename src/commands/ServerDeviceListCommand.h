#pragma once

#include "core/Command.h"
#include "model/DevicePrefix.h"

namespace BeeeOn {

/*
 * List of joined devices serves for device discovery,
 * with which this device manager (identified by DevicePrefix)
 * can communicate. This information should not be saved
 * directly on gateway.
 */
class ServerDeviceListCommand : public Command {
public:
	typedef Poco::AutoPtr<ServerDeviceListCommand> Ptr;

	ServerDeviceListCommand(const DevicePrefix &prefix);

	DevicePrefix devicePrefix() const;

	std::string toString() const override;

protected:
	~ServerDeviceListCommand();

private:
	DevicePrefix m_prefix;
};

}
