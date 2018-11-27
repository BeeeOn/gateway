#include <Poco/DateTimeFormatter.h>
#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "core/DevicePoller.h"
#include "di/Injectable.h"

BEEEON_OBJECT_BEGIN(BeeeOn, DevicePoller)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_PROPERTY("distributor", &DevicePoller::setDistributor)
BEEEON_OBJECT_PROPERTY("pollExecutor", &DevicePoller::setPollExecutor)
BEEEON_OBJECT_PROPERTY("warnThreshold", &DevicePoller::setWarnThreshold)
BEEEON_OBJECT_HOOK("cleanup", &DevicePoller::cleanup)
BEEEON_OBJECT_END(BeeeOn, DevicePoller)

using namespace Poco;
using namespace BeeeOn;

DevicePoller::DevicePoller():
	m_warnThreshold(1 * Timespan::SECONDS)
{
}

void DevicePoller::setDistributor(Distributor::Ptr distributor)
{
	m_distributor = distributor;
}

void DevicePoller::setPollExecutor(AsyncExecutor::Ptr executor)
{
	m_pollExecutor = executor;
}

void DevicePoller::setWarnThreshold(const Poco::Timespan &threshold)
{
	m_warnThreshold = threshold;
}

Timespan DevicePoller::grabRefresh(const PollableDevice::Ptr device)
{
	const auto refresh = device->refresh();

	if (refresh.isNone() || refresh.isDisabled()) {
		throw IllegalStateException(
			"device " + device->id().toString()
			+ " is not pollable due to its refresh settings");
	}

	return refresh.time();
}

void DevicePoller::schedule(
		PollableDevice::Ptr device,
		const Clock &now)
{
	FastMutex::ScopedLock guard(m_lock);

	if (m_active.find(device->id()) != end(m_active))
		return;
	if (m_devices.find(device->id()) != end(m_devices))
		return;

	doSchedule(device, now);

	if (logger().debug()) {
		logger().debug(
			"scheduled device " + device->id().toString()
			+ " for polling",
			__FILE__, __LINE__);
	}
}

void DevicePoller::doSchedule(
		PollableDevice::Ptr device,
		const Clock &now)
{
	const auto refresh = grabRefresh(device);
	const auto next = now + refresh.totalMicroseconds();

	auto it = m_schedule.emplace(next, device);
	auto r = m_devices.emplace(device->id(), it);

	if (!r.second) { // update existing if it was there
		m_schedule.erase(r.first->second);
		r.first->second = it;
	}

	m_stopControl.requestWakeup();
}

void DevicePoller::cancel(const DeviceID &id)
{
	FastMutex::ScopedLock guard(m_lock);

	m_active.erase(id); // avoid rescheduling

	auto it = m_devices.find(id);
	if (it == end(m_devices))
		return;

	m_schedule.erase(it->second);
	m_devices.erase(it);

	if (logger().debug()) {
		logger().debug(
			"cancelling device " + id.toString()
			+ " from polling",
			__FILE__, __LINE__);
	}
}

void DevicePoller::reschedule(
		PollableDevice::Ptr device,
		const Clock &now)
{
	FastMutex::ScopedLock guard(m_lock);

	auto it = m_active.find(device->id());
	if (it == end(m_active))
		return; // only active devices can be rescheduled

	m_active.erase(it);

	try {
		doSchedule(device, now);
	}
	BEEEON_CATCH_CHAIN_ACTION(logger(),
		return)

	if (logger().debug()) {
		logger().debug(
			"rescheduled device " + device->id().toString()
			+ " for polling",
			__FILE__, __LINE__);
	}
}

void DevicePoller::run()
{
	StopControl::Run run(m_stopControl);

	logger().information("device poller is starting");

	while (run) {
		ScopedLockWithUnlock<FastMutex> guard(m_lock);

		if (m_schedule.empty()) {
			if (!m_active.empty()) {
				if (logger().debug()) {
					logger().debug(
						"all devices are active, sleeping",
						__FILE__, __LINE__);
				}
			}
			else {
				if (logger().debug()) {
					logger().debug(
						"no device to poll, sleeping",
						__FILE__, __LINE__);
				}
			}

			guard.unlock();
			m_stopControl.waitStoppable(-1);
			continue;
		}

		const auto &sleep = pollNextIfOnSchedule();
		if (sleep > 0) {
			guard.unlock();

			if (logger().debug()) {
				logger().debug(
					"no device to poll now, sleeping for at least "
					+ DateTimeFormatter::format(sleep, "%h:%M:%S.%i"),
					__FILE__, __LINE__);
			}

			m_stopControl.waitStoppable(sleep);
			continue;
		}
	}

	logger().information("device poller has stopped");
}

Timespan DevicePoller::pollNextIfOnSchedule(const Clock &now)
{
	auto it = begin(m_schedule);
	poco_assert(it != end(m_schedule));

	if (it->first > now)
		return it->first - now;

	PollableDevice::Ptr device = it->second;

	m_devices.erase(device->id());
	m_active.emplace(device->id());
	m_schedule.erase(it);

	doPoll(device);
	return 0;
}

void DevicePoller::doPoll(PollableDevice::Ptr device)
{
	m_pollExecutor->invoke([&, device]() mutable {
		const Clock started;

		if (logger().debug()) {
			logger().debug(
				"polling device " + device->id().toString(),
				__FILE__, __LINE__);
		}

		try {
			device->poll(m_distributor);
		}
		BEEEON_CATCH_CHAIN(logger())

		const Timespan elapsed = started.elapsed();
		const auto diff = elapsed - device->refresh();

		if (diff > m_warnThreshold) {
			logger().warning(
				"polling of " + device->id().toString()
				+ " took too long ("
				+ DateTimeFormatter::format(elapsed, "%h:%M:%S.%i")
				+ ") with respect to refresh time ("
				+ DateTimeFormatter::format(device->refresh(), "%h:$M:%S")
				+ ")",
				__FILE__, __LINE__);
		}

		reschedule(device);
	});
}

void DevicePoller::stop()
{
	m_stopControl.requestStop();
}

void DevicePoller::cleanup()
{
	FastMutex::ScopedLock guard(m_lock);

	m_active.clear();
	m_devices.clear();
	m_schedule.clear();
	m_pollExecutor = nullptr;
}
