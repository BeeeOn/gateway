#include <Poco/DirectoryIterator.h>
#include <Poco/Logger.h>
#include <Poco/NamedMutex.h>

#include "core/FilesystemDeviceCache.h"
#include "di/Injectable.h"

BEEEON_OBJECT_BEGIN(BeeeOn, FilesystemDeviceCache)
BEEEON_OBJECT_CASTABLE(DeviceCache)
BEEEON_OBJECT_PROPERTY("cacheDir", &FilesystemDeviceCache::setCacheDir)
BEEEON_OBJECT_END(BeeeOn, FilesystemDeviceCache)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

FilesystemDeviceCache::FilesystemDeviceCache():
	m_cacheDir("/var/cache/beeeon/gateway/devices")
{
}

void FilesystemDeviceCache::setCacheDir(const string &dir)
{
	m_cacheDir = dir;
}

File FilesystemDeviceCache::locatePrefix(const DevicePrefix &prefix) const
{
	const Path prefixDir(m_cacheDir, prefix.toString());
	return File(prefixDir);
}

File FilesystemDeviceCache::locateID(const DeviceID &id) const
{
	const Path prefixDir(m_cacheDir, id.prefix().toString());
	const Path idPath(prefixDir, id.toString());
	return File(idPath);
}

bool FilesystemDeviceCache::decodeName(
		const string &name,
		DeviceID &id) const
{
	try {
		id = DeviceID::parse(name);
		return true;
	}
	catch (const LogicException &e) {
		logger().warning(e.displayText(), __FILE__, __LINE__);
	}
	BEEEON_CATCH_CHAIN(logger())

	return false;
}

void FilesystemDeviceCache::drop(const DeviceID &id) const
{
	File file = locateID(id);

	if (logger().trace())
		logger().trace("dropping " + file.path(), __FILE__, __LINE__);

	try {
		file.remove(true);

		logger().information("file " + file.path() + " was deleted",
			__FILE__, __LINE__);
	}
	catch (const FileNotFoundException &e) {
		if (logger().debug())
			logger().debug(e.displayText(), __FILE__, __LINE__);
	}
	BEEEON_CATCH_CHAIN(logger())
}

void FilesystemDeviceCache::write(const DeviceID &id) const
{
	File file = locateID(id);

	if (logger().trace()) {
		logger().trace(
			"writing " + file.path(), __FILE__, __LINE__);
	}

	try {
		if (file.createFile()) {
			logger().information(
				"file " + file.path() + " was created",
				__FILE__, __LINE__);
		}
		else if (logger().debug()) {
			logger().debug("file " + file.path() + " already exists",
				__FILE__, __LINE__);
		}
	}
	BEEEON_CATCH_CHAIN(logger())
}

void FilesystemDeviceCache::markPaired(
		const DevicePrefix &prefix,
		const set<DeviceID> &devices)
{
	NamedMutex lock(prefix.toString());
	NamedMutex::ScopedLock guard(lock);

	File prefixFile = locatePrefix(prefix);
	prefixFile.createDirectories();

	DirectoryIterator it(prefixFile);
	const DirectoryIterator end;

	if (logger().debug()) {
		logger().debug("saving " + prefix.toString() + " into " + prefixFile.path(),
				__FILE__, __LINE__);
	}

	for (; it != end; ++it) {
		if (logger().trace())
			logger().trace("visiting " + it->path(), __FILE__, __LINE__);

		DeviceID id;
		if (!decodeName(it.name(), id))
			continue;

		if (devices.find(id) == devices.end())
			drop(id);
	}

	for (const auto &id : devices) {
		if (id.prefix() != prefix) {
			logger().warning(
				"skipping ID " + id.toString()
				+ " of unexpected prefix " + id.prefix(),
				__FILE__, __LINE__);
			continue;
		}

		write(id);
	}
}

void FilesystemDeviceCache::markPaired(
		const DeviceID &id)
{
	NamedMutex lock(id.prefix().toString());
	NamedMutex::ScopedLock guard(lock);

	File prefixFile = locatePrefix(id.prefix());
	prefixFile.createDirectories();

	write(id);
}

void FilesystemDeviceCache::markUnpaired(
		const DeviceID &id)
{
	NamedMutex lock(id.prefix().toString());
	NamedMutex::ScopedLock guard(lock);

	const File &prefixFile = locatePrefix(id.prefix());
	if (!prefixFile.exists())
		return;

	drop(id);
}

bool FilesystemDeviceCache::paired(const DeviceID &id) const
{
	NamedMutex lock(id.prefix().toString());
	NamedMutex::ScopedLock guard(lock);

	const File &file = locateID(id);
	return file.exists();
}

set<DeviceID> FilesystemDeviceCache::paired(const DevicePrefix &prefix) const
{
	NamedMutex lock(prefix.toString());
	NamedMutex::ScopedLock guard(lock);

	const File &prefixFile = locatePrefix(prefix);
	if (!prefixFile.exists())
		return {};

	DirectoryIterator it(prefixFile);
	const DirectoryIterator end;

	set<DeviceID> devices;

	for (; it != end; ++it) {
		if (logger().trace())
			logger().trace("visiting " + it->path(), __FILE__, __LINE__);

		DeviceID id;
		if (!decodeName(it.name(), id))
			continue;

		if (id.prefix() != prefix) {
			logger().warning(
				"skipping ID " + id.toString()
				+ " of unexpected prefix " + id.prefix(),
				__FILE__, __LINE__);
			continue;
		}

		devices.emplace(id);
	}

	return devices;
}
