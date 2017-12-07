#include "server/GWSOutputQueue.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

GWSOutputQueue::GWSOutputQueue(Event &enqueueEvent):
	m_enqueueEvent(enqueueEvent)
{
}

GWSOutputQueue::~GWSOutputQueue()
{
	clear();
}

void GWSOutputQueue::enqueue(GWMessageContext::Ptr context)
{
	FastMutex::ScopedLock guard(m_mutex);
	m_queue.push(context);
	m_enqueueEvent.set();
}

GWMessageContext::Ptr GWSOutputQueue::dequeue()
{
	GWMessageContext::Ptr context;

	FastMutex::ScopedLock guard(m_mutex);

	if (!m_queue.empty()) {
		context = m_queue.top();
		m_queue.pop();
	}

	return context;
}

void GWSOutputQueue::clear()
{
	FastMutex::ScopedLock guard(m_mutex);

	poco_debug(logger(), "clearing queue with " + to_string(m_queue.size()) + " contexts enqueued");

	while(!m_queue.empty()) {
		GWMessageContext::Ptr context = m_queue.top();
		poco_debug(logger(), "clearing context id: " + context->id().toString()
				+ " type: " + context->message()->type().toString());
		m_queue.pop();
	}
}
