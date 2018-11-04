#pragma once

#include <deque>

#include <Poco/Event.h>
#include <Poco/Mutex.h>

#include "util/Loggable.h"
#include "zwave/ZWaveNetwork.h"

namespace BeeeOn {

/**
 * @brief Abstract implementation of the ZWaveNetwork class. It provides
 * a pre-implemented polling mechanism. It is assumed that exactly one
 * thread calls the method pollEvent() periodically to read the events
 * (using multiple threads might be an issue because we use Poco::Event).
 */
class AbstractZWaveNetwork :
	public ZWaveNetwork,
	protected virtual Loggable {
public:
	AbstractZWaveNetwork();

	/**
	 * Implements the pollEvent() operation generically. It just
	 * waits on the m_event and reads events from the m_eventsQueue.
	 */
	PollEvent pollEvent(
		const Poco::Timespan &timeout) override;

	/**
	 * Interrupt the pollEvent() operation to return regardless of
	 * the state of the m_eventsQueue.
	 */
	void interrupt() override;

protected:
	/**
	 * This method enqueues the given event in the m_eventsQueue and
	 * wake ups the pollEvent() operation.
	 */
	void notifyEvent(const PollEvent &event);

private:
	std::deque<PollEvent> m_eventsQueue;
	Poco::Event m_event;
	mutable Poco::FastMutex m_lock;
};

}
