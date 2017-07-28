#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "core/DongleDeviceManager.h"
#include "udev/UDevEvent.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

DongleDeviceManager::DongleDeviceManager(const DevicePrefix &prefix):
	DeviceManager(prefix)
{
}

bool DongleDeviceManager::dongleMissing()
{
	return true;
}

string DongleDeviceManager::dongleName(bool failWhenMissing) const
{
	FastMutex::ScopedLock guard(m_lock);

	if (m_dongleName.empty() && failWhenMissing)
		throw IOException("dongle seems to be disconnected");

	return m_dongleName;
}

Event &DongleDeviceManager::event()
{
	return m_event;
}

void DongleDeviceManager::onAdd(const UDevEvent &e)
{
	FastMutex::ScopedLock guard(m_lock);

	if (!m_dongleName.empty()) {
		logger().trace("ignored event " + e.toString(),
				__FILE__, __LINE__);
		return;
	}

	const string &name = dongleMatch(e);
	if (name.empty()) {
		logger().trace("event " + e.toString() + " does not match",
				__FILE__, __LINE__);
		return;
	}

	logger().debug("registering dongle " + e.toString(),
			__FILE__, __LINE__);

	m_dongleName = (name);
	event().set();
}

void DongleDeviceManager::onRemove(const UDevEvent &e)
{
	FastMutex::ScopedLock guard(m_lock);

	if (m_dongleName.empty()) {
		logger().trace("ignored event " + e.toString(),
				__FILE__, __LINE__);
		return;
	}

	const string &name = dongleMatch(e);
	if (name.empty()) {
		logger().trace("event " + e.toString() + " does not match",
				__FILE__, __LINE__);
		return;
	}

	logger().debug("unregistering dongle " + e.toString(),
			__FILE__, __LINE__);

	m_dongleName.clear();
	event().set();
}

void DongleDeviceManager::run()
{
	logger().information("device manager is starting",
			     __FILE__, __LINE__);

	while (!m_stop) {
		while (!m_stop && dongleName(false).empty()) {
			logger().information("no appropriate dongle is available",
					     __FILE__, __LINE__);

			if (dongleMissing())
				event().wait();
		}

		if (m_stop)
			break;

		try {
			logger().information(
				"dongle is available: " + dongleName(),
					     __FILE__, __LINE__);

			dongleAvailable();
			break;
		}
		catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
			continue;
		}
		catch (const exception &e) {
			logger().critical(e.what(), __FILE__, __LINE__);
			continue;
		}
		catch (...) {
			logger().critical("unknown error", __FILE__, __LINE__);
			continue;
		}
	}

	logger().information("device manager has finished",
			     __FILE__, __LINE__);

	m_stop = false;
}

void DongleDeviceManager::stop()
{
	DeviceManager::stop();
	event().set();
}
