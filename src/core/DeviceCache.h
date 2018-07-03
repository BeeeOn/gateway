#pragma once

#include <set>

#include <Poco/SharedPtr.h>

#include "model/DeviceID.h"
#include "model/DevicePrefix.h"

namespace BeeeOn {

/**
 * @brief DeviceCache manages pairing status devices which allows to choose
 * different caching strategies like: in-memory cache, persistent cache,
 * centralized cache, etc.
 */
class DeviceCache {
public:
	typedef Poco::SharedPtr<DeviceCache> Ptr;

	virtual ~DeviceCache();

	/**
	 * Mark paired all the given devices with the prefix.
	 * Devices that are not included in the set are marked
	 * as unpaired. No other device of the prefix except those
	 * in the given set would be paired after this operation.
	 */
	virtual void markPaired(
		const DevicePrefix &prefix,
		const std::set<DeviceID> &devices) = 0;

	/**
	 * Mark the device as paired.
	 */
	virtual void markPaired(const DeviceID &device) = 0;

	/**
	 * Mark the device as unpaired.
	 */
	virtual void markUnpaired(const DeviceID &device) = 0;

	/**
	 * @returns true if such device is marked as paired
	 */
	virtual bool paired(const DeviceID &id) const = 0;

	/**
	 * Provide set of all paired devices for the given prefix.
	 */
	virtual std::set<DeviceID> paired(const DevicePrefix &prefix) const = 0;
};

}
