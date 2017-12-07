#pragma once

#include <queue>

#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>

#include "server/GWMessageContext.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief Queue for all outgoing messages. Must be initialized with
 * Poco::Event reference, which is notified on item enqueue.
 */
class GWSOutputQueue : public Loggable {
public:
	GWSOutputQueue(Poco::Event &enqueueEvent);
	virtual ~GWSOutputQueue();

	void enqueue(GWMessageContext::Ptr context);

	GWMessageContext::Ptr dequeue();

	void clear();

private:
	std::priority_queue<GWMessageContext::Ptr,
			std::vector<GWMessageContext::Ptr>,
			ContextPriorityComparator> m_queue;

	Poco::FastMutex m_mutex;
	Poco::Event &m_enqueueEvent;
};

}
