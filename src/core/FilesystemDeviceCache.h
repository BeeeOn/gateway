#pragma once

#include <string>

#include <Poco/File.h>
#include <Poco/Path.h>

#include "core/DeviceCache.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief FilesystemDeviceCache implements DeviceCache by creating
 * and removing files inside a filesystem directory. Each paired
 * device has a file named by its ID created. Devices are categorized
 * by the ID prefix.
 *
 * The cache is created under the directory specified by property
 * <code>cacheDir</code>. If such path does not exist, it is created
 * when on demand.
 *
 * The FilesystemDeviceCache uses global locking (Poco::NamedLock)
 * for each set of ID of the same prefix. Such lock is named after
 * the prefix.
 */
class FilesystemDeviceCache :
	public DeviceCache,
	Loggable {
public:
	FilesystemDeviceCache();

	void setCacheDir(const std::string &path);

	/**
	 * @brief Synchronize the contents of <code>$cacheDir/$prefix</code> with the
	 * given set of devices.
	 *
	 * Existing files <code>$cacheDir/$prefix/$id</code> are preserved if they
	 * are present in the given set of devices, otherwise they are deleted.
	 * Missing files <code>$cacheDir/$prefix/$id</code> that are contained
	 * inside the given set of devices are created.
	 *
	 * The operation is atomic from the application point of view.
	 * It is however not guaranteed that the state of the cache is consistent
	 * with the given set of devices when a serious failure occures.
	 */
	void markPaired(
		const DevicePrefix &prefix,
		const std::set<DeviceID> &devices) override;

	/**
	 * @brief Create file <code>$cacheDir/$prefix/$id</code> (if does not exist).
	 */
	void markPaired(const DeviceID &device) override;

	/**
	 * @brief Remove file <code>$cacheDir/$prefix/$id</code> (if exists).
	 */
	void markUnpaired(const DeviceID &device) override;

	/**
	 * @returns true if the file <code>$cacheDir/$prefix/$id</code> exists
	 */
	bool paired(const DeviceID &device) const override;

	/**
	 * @returns set of file names found in <code>$cacheDir/$prefix</code> that
	 * are represent valid device IDs
	 */
	std::set<DeviceID> paired(const DevicePrefix &prefix) const override;

protected:
	/**
	 * @returns Poco::File pointing to the directory where
	 * IDs of the given prefix are stored.
	 */
	Poco::File locatePrefix(const DevicePrefix &prefix) const;

	/**
	 * @returns Poco::File pointing to the file representing the given ID
	 */
	Poco::File locateID(const DeviceID &id) const;

	/**
	 * @brief Parse file name and try to decode it as it is a device ID.
	 * The result is given in the referenced parameter id.
	 * No exceptions should be thrown.
	 *
	 * @returns true if the decoding was successful
	 */
	bool decodeName(const std::string &name, DeviceID &id) const;

	/**
	 * @brief Remove file <code>$cacheDir/$prefix/$id</code> from the filesystem
	 * if it does exist. No exceptions should be thrown.
	 */
	void drop(const DeviceID &id) const;

	/**
	 * @brief Create file <code>$cacheDir/$prefix/$id</code> in the filesystem
	 * if it does not exist. No exceptions should be thrown.
	 */
	void write(const DeviceID &id) const;

private:
	Poco::Path m_cacheDir;
};

}
