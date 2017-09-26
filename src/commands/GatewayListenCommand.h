#ifndef BEEEON_GATEWAY_LISTEN_COMMAND_H
#define BEEEON_GATEWAY_LISTEN_COMMAND_H

#include <Poco/Timespan.h>

#include "core/Command.h"

namespace BeeeOn {

/*
 * When the device manager receives this message, it is
 * set to the status when new devices in the network
 * can appear. Timeout defines time during which new
 * devices can occur.
 */
class GatewayListenCommand : public Command {
public:
	typedef Poco::AutoPtr<GatewayListenCommand> Ptr;

	GatewayListenCommand(const Poco::Timespan &duration);

	Poco::Timespan duration() const;

	std::string toString() const override;

protected:
	~GatewayListenCommand();

private:
	Poco::Timespan m_duration;
};

}

#endif
