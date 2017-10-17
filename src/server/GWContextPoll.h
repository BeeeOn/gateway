#pragma once

#include <map>

#include <Poco/Mutex.h>

#include "server/GWMessageContext.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief GWContextPoll stores contexts of sent messages. This is used
 * for messages that expects the answer, so they can be matched. All
 * supported operations are thread-safe.
 */
class GWContextPoll : public Loggable {
public:
	virtual ~GWContextPoll();

	void insert(GWMessageContext::Ptr context);

	GWMessageContext::Ptr remove(const GlobalID &id);

	void clear();

private:
	std::map<GlobalID, GWMessageContext::Ptr> m_messages;
	Poco::FastMutex m_mutex;
};

}
