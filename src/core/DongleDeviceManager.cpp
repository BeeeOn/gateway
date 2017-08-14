#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "core/DongleDeviceManager.h"
#include "hotplug/HotplugEvent.h"
#include "util/FailDetector.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

DongleDeviceManager::DongleDeviceManager(const DevicePrefix &prefix):
	DeviceManager(prefix),
	m_attemptsCount(3),
	m_retryTimeout(10 * Timespan::SECONDS),
	m_failTimeout(1 * Timespan::SECONDS)
{
}

void DongleDeviceManager::setAttemptsCount(const int count)
{
	if (count <= 0)
		throw InvalidAccessException("attemptsCount must be greater then 0");

	m_attemptsCount = count;
}

void DongleDeviceManager::setRetryTimeout(const int msecs)
{
	m_retryTimeout = msecs * Timespan::MILLISECONDS;
}

void DongleDeviceManager::setFailTimeout(const int msecs)
{
	if (msecs < 0)
		throw InvalidArgumentException("failTimeout must not be negative");

	m_failTimeout = msecs * Timespan::MILLISECONDS;
}

bool DongleDeviceManager::dongleMissing()
{
	return true;
}

void DongleDeviceManager::dongleFailed(const FailDetector &)
{
	logger().critical("dongle seems to be failing",
			__FILE__, __LINE__);

	if (m_retryTimeout < 0) {
		// wait indefinitely until an event occures
		event().wait();
	}
	else {
		// wait for the retry timeout
		(void) event().tryWait(m_retryTimeout.totalMilliseconds());
	}
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

void DongleDeviceManager::onAdd(const HotplugEvent &e)
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

void DongleDeviceManager::onRemove(const HotplugEvent &e)
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

	FailDetector dongleStatus(m_attemptsCount);

	while (!m_stop) {
		while (!m_stop && dongleName(false).empty()) {
			logger().information("no appropriate dongle is available",
					     __FILE__, __LINE__);

			if (dongleMissing())
				event().wait();

			dongleStatus.success();
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
		}
		catch (const exception &e) {
			logger().critical(e.what(), __FILE__, __LINE__);
		}
		catch (...) {
			logger().critical("unknown error", __FILE__, __LINE__);
		}

		dongleStatus.fail();

		if (dongleStatus.isFailed()) {
			dongleFailed(dongleStatus);
			dongleStatus.success();
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
