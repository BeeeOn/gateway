#pragma once

#include <list>
#include <map>
#include <string>

#include <Poco/RWLock.h>

#include "core/DeviceCache.h"

namespace BeeeOn {

/**
 * @brief MemoryDeviceCache implements in-memory volatile DeviceCache.
 */
class MemoryDeviceCache : public DeviceCache {
public:
	/**
	 * @brief Set devices to be initially marked as paired. This
	 * feature might be useful for debugging purposes.
	 */
	void setPrepaired(const std::list<std::string> &devices);

	/**
	 * @brief Stores the given set of devices of the same prefix
	 * as the set of paired devices. Devices not included
	 * in the set would be marked as unpaired.
	 */
	void markPaired(
		const DevicePrefix &prefix,
		const std::set<DeviceID> &devices) override;

	/**
	 * @brief Store the given device id as a paired device.
	 * If the id's prefix is not recorded yet, it would be created
	 * and it would contain only the given id.
	 */
	void markPaired(const DeviceID &id) override;

	/**
	 * @brief Remove the given id from the cache. If the id's prefix
	 * does not contain any other id, it would be removed as well.
	 */
	void markUnpaired(const DeviceID &id) override;

	/**
	 * @returns whether the given id has a record in the cache
	 */
	bool paired(const DeviceID &id) const override;

	/**
	 * Provide the set of paired devices of the particular prefix.
	 */
	std::set<DeviceID> paired(const DevicePrefix &prefix) const override;

private:
	std::map<DevicePrefix, std::set<DeviceID>> m_cache;
	mutable Poco::RWLock m_lock;
};

}
