#include "core/MemoryDeviceCache.h"
#include "di/Injectable.h"

BEEEON_OBJECT_BEGIN(BeeeOn, MemoryDeviceCache)
BEEEON_OBJECT_CASTABLE(DeviceCache)
BEEEON_OBJECT_PROPERTY("prepaired", &MemoryDeviceCache::setPrepaired)
BEEEON_OBJECT_END(BeeeOn, MemoryDeviceCache)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

void MemoryDeviceCache::setPrepaired(const list<string> &devices)
{
	for (const auto &one : devices) {
		const auto &id = DeviceID::parse(one);
		markPaired(id);
	}
}

void MemoryDeviceCache::markPaired(
		const DevicePrefix &prefix,
		const set<DeviceID> &devices)
{
	RWLock::ScopedWriteLock guard(m_lock);

	auto result = m_cache.emplace(prefix, devices);
	if (result.second)
		return;

	auto &prefixCache = result.first->second;
	prefixCache = devices;
}

void MemoryDeviceCache::markPaired(const DeviceID &id)
{
	RWLock::ScopedWriteLock guard(m_lock);

	const set<DeviceID> &single = {id};
	auto result = m_cache.emplace(id.prefix(), single);
	if (result.second)
		return;

	auto &prefixCache = result.first->second;
	prefixCache.emplace(id);
}

void MemoryDeviceCache::markUnpaired(const DeviceID &id)
{
	RWLock::ScopedWriteLock guard(m_lock);

	auto it = m_cache.find(id.prefix());
	if (it == m_cache.end())
		return;

	it->second.erase(id);

	if (it->second.empty())
		m_cache.erase(it);
}

bool MemoryDeviceCache::paired(const DeviceID &id) const
{
	RWLock::ScopedReadLock guard(m_lock);

	auto it = m_cache.find(id.prefix());
	if (it == m_cache.end())
		return false;

	return it->second.find(id) != it->second.end();
}

set<DeviceID> MemoryDeviceCache::paired(const DevicePrefix &prefix) const
{
	RWLock::ScopedReadLock guard(m_lock);

	auto it = m_cache.find(prefix);
	if (it == m_cache.end())
		return {};

	return it->second;
}
