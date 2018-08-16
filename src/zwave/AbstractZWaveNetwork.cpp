#include <Poco/Clock.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "zwave/AbstractZWaveNetwork.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

AbstractZWaveNetwork::AbstractZWaveNetwork()
{
}

ZWaveNetwork::PollEvent AbstractZWaveNetwork::pollEvent(
		const Timespan &timeout)
{
	const Clock started;

	PollEvent event;
	bool mightBlock = true;

	do {
		ScopedLockWithUnlock<FastMutex> guard(m_lock);

		if (logger().trace()) {
			logger().trace(
				"polling attempt, queue depth: " + to_string(m_eventsQueue.size())
				+ (mightBlock? " (might block)" : " (might not block)"),
				__FILE__, __LINE__);
		}


		if (!m_eventsQueue.empty()) {
			event = m_eventsQueue.front();
			m_eventsQueue.pop_front();
			break;
		}

		guard.unlock();

		if (logger().trace()) {
			logger().trace(
				"polling queue is empty",
				__FILE__, __LINE__);
		}

		if (mightBlock && timeout >= 1 * Timespan::MILLISECONDS) {
			Timespan remaining = timeout - started.elapsed();

			if (remaining < 0)
				break;
			if (remaining < 1 * Timespan::MILLISECONDS)
				remaining = 1 * Timespan::MILLISECONDS;

			if (logger().trace()) {
				logger().trace(
					"sleeping while polling for "
					+ DateTimeFormatter::format(remaining),
					__FILE__, __LINE__);
			}

			if (m_event.tryWait(remaining.totalMilliseconds()))
				mightBlock = false;
		}
		else if (mightBlock && timeout < 0) {
			if (logger().trace()) {
				logger().trace(
					"sleeping while polling...",
					__FILE__, __LINE__);
			}

			m_event.wait();
			mightBlock = false;
		}
		else {
			mightBlock = false;
		}
	} while(1);

	return event;
}

void AbstractZWaveNetwork::notifyEvent(const PollEvent &event)
{
	FastMutex::ScopedLock guard(m_lock);

	m_eventsQueue.emplace_back(event);
	m_event.set();
}

void AbstractZWaveNetwork::interrupt()
{
	FastMutex::ScopedLock guard(m_lock);

	if (logger().trace()) {
		logger().trace(
			"interrupting pollers, queue depth: "
			+ to_string(m_eventsQueue.size()),
			__FILE__, __LINE__);
	}

	PollEvent none;
	m_eventsQueue.emplace_back(none);
	m_event.set();
}
