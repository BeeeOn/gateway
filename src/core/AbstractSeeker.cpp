#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "core/AbstractSeeker.h"

using namespace Poco;
using namespace BeeeOn;

AbstractSeeker::AbstractSeeker(const Timespan &duration):
	m_duration(duration),
	m_runnable(*this, &AbstractSeeker::seek),
	m_hasStarted(false),
	m_joiner(m_thread)
{
}

Timespan AbstractSeeker::duration() const
{
	return m_duration;
}

Timespan AbstractSeeker::elapsed() const
{
	FastMutex::ScopedLock guard(m_lock);

	if (m_hasStarted)
		return m_started.elapsed();

	return 0;
}

Timespan AbstractSeeker::remaining() const
{
	FastMutex::ScopedLock guard(m_lock);

	if (m_hasStarted && !m_thread.isRunning())
		return 0;

	else if (!m_hasStarted)
		return m_duration;

	const auto diff = m_duration - m_started.elapsed();
	if (diff < 0)
		return 0;

	return diff;
}

void AbstractSeeker::start()
{
	FastMutex::ScopedLock guard(m_lock);

	if (m_thread.isRunning())
		throw IllegalStateException("seeker is already running");

	if (m_hasStarted)
		throw IllegalStateException("seeker cannot be started twice");

	m_thread.start(m_runnable);
}

bool AbstractSeeker::tryJoin(const Timespan &timeout)
{
	return m_joiner.tryJoin(timeout);
}

void AbstractSeeker::cancel()
{
	m_stopControl.requestStop();
	m_joiner.join();
}

void AbstractSeeker::seek()
{
	{
		FastMutex::ScopedLock guard(m_lock);
		m_started.update();
		m_hasStarted = true;
	}

	seekLoop(m_stopControl);
}
