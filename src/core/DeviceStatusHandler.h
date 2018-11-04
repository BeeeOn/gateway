#pragma once

#include <set>
#include <map>

#include <Poco/SharedPtr.h>

#include "model/DeviceID.h"
#include "model/DevicePrefix.h"
#include "model/ModuleID.h"

namespace BeeeOn {

/**
 * @brief DeviceStatusHandler represents a class that can process
 * status of a set of devices. This is useful when fetching pairing
 * state from a remote server.
 */
class DeviceStatusHandler {
public:
	typedef Poco::SharedPtr<DeviceStatusHandler> Ptr;
	typedef std::map<DeviceID, std::map<ModuleID, double>> DeviceValues;

	virtual ~DeviceStatusHandler();

	/**
	 * @return device prefix the handler would like handle
	 */
	virtual DevicePrefix prefix() const = 0;

	/**
	 * @brief Handle device status as understood by a remote server.
	 * All devices of a certain prefix are notified in this way.
	 *
	 * The set of paired devices represents all paired devices of
	 * the common prefix. All missing devices should be threated
	 * as unpaired for that prefix. Also, the most recent status
	 * of certain devices can be passed this way.
	 */
	virtual void handleRemoteStatus(
		const DevicePrefix &prefix,
		const std::set<DeviceID> &paired,
		const DeviceValues &values) = 0;
};

}
