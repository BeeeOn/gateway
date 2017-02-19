#ifndef BEEEON_INCOMPLETE_TIMESTAMP_H
#define BEEEON_INCOMPLETE_TIMESTAMP_H

#include <ostream>

#include <Poco/Timespan.h>
#include <Poco/Timestamp.h>

#include "util/Incomplete.h"

namespace BeeeOn {

struct TimestampCompleteTest {
	// Thu Jan  1 01:00:00 CET 2015
	static const unsigned long TOO_OLD = 1420070400UL;

	bool operator ()(const Poco::Timestamp &t) const
	{
		static const Poco::Timestamp tooOld(
			Poco::Timestamp::fromEpochTime(TOO_OLD));

		return t > tooOld;
	}
};

struct TimestampComplete {
	Poco::Timestamp operator ()(const Poco::Timestamp &t) const
	{
		return m_now - m_uptime + t.epochMicroseconds();
	}

	Poco::Timespan m_uptime;
	Poco::Timestamp m_now;
};

/**
 * Shortcut to represent an incomplete timestamp.
 */
typedef Incomplete<
	Poco::Timestamp, TimestampCompleteTest, TimestampComplete
> IncompleteTimestamp;

};

#endif
