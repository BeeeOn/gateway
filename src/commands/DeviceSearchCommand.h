#pragma once

#include <string>

#include <Poco/Nullable.h>
#include <Poco/Timespan.h>

#include <Poco/Net/IPAddress.h>

#include "core/PrefixCommand.h"
#include "model/DeviceCriteria.h"
#include "model/DevicePrefix.h"
#include "net/MACAddress.h"

namespace BeeeOn {

/**
 * @brief DeviceSearchCommand requests a device manager to start
 * searching for a single device specified by either its:
 *
 * - IP address
 * - MAC address
 * - serial number
 *
 * This command is not mandatory to be supported.
 */
class DeviceSearchCommand : public PrefixCommand {
public:
	typedef Poco::AutoPtr<DeviceSearchCommand> Ptr;

	DeviceSearchCommand(
		const DevicePrefix &prefix,
		const DeviceCriteria &criteria,
		const Poco::Timespan &duration);

	DeviceCriteria criteria() const;

	bool hasIPAddress() const;
	Poco::Net::IPAddress ipAddress() const;

	bool hasMACAddress() const;
	MACAddress macAddress() const;

	bool hasSerialNumber() const;
	uint64_t serialNumber() const;

	Poco::Timespan duration() const;

	std::string toString() const override;

private:
	DeviceCriteria m_criteria;
	Poco::Timespan m_duration;
};

}
