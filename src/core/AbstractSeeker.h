#pragma once

#include <Poco/Clock.h>
#include <Poco/Mutex.h>
#include <Poco/RunnableAdapter.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/Thread.h>

#include "util/AsyncWork.h"
#include "util/Joiner.h"
#include "util/Loggable.h"
#include "loop/StopControl.h"

namespace BeeeOn {

/**
 * @brief AbstractSeeker represents an asynchronous process that seeks
 * for new devices in a certain network. It is basically a thread that
 * performs some technology-specific routines to discover new devices.
 *
 * A single AbstractSeeker instance can perform only 1 seek. For every
 * other seek a new AbstractSeeker must be created.
 */
class AbstractSeeker : public AsyncWork<>, protected Loggable {
public:
	typedef Poco::SharedPtr<AbstractSeeker> Ptr;

	AbstractSeeker(const Poco::Timespan &duration);

	/**
	 * @returns total duration of seeking
	 */
	Poco::Timespan duration() const;

	/**
	 * @return time elapsed while seeking
	 */
	Poco::Timespan elapsed() const;

	/**
	 * @brief Compute time that is remaining to finish
	 * the seeking process. If the seeking has finished,
	 * it returns 0.
	 *
	 * @return time remaining to finish of seeking
	 */
	Poco::Timespan remaining() const;

	/**
	 * @brief Start the seeking thread.
	 * @throws Poco::IllegalStateException in case the seeker was already started.
	 */
	void start();

	/**
	 * @brief Join the seeking thread via Joiner.
	 */
	bool tryJoin(const Poco::Timespan &timeout) override;

	/**
	 * @brief Cancel seeking and wait for the thread to finish.
	 */
	void cancel() override;

protected:
	void seek();
	virtual void seekLoop(StopControl &control) = 0;

private:
	Poco::Timespan m_duration;
	Poco::RunnableAdapter<AbstractSeeker> m_runnable;
	Poco::Thread m_thread;
	mutable Poco::FastMutex m_lock;
	Poco::Clock m_started;
	bool m_hasStarted;
	StopControl m_stopControl;
	Joiner m_joiner;
};

}
