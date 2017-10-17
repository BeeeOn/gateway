#include "server/GWContextPoll.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

GWContextPoll::~GWContextPoll()
{
	clear();
}

void GWContextPoll::insert(GWMessageContext::Ptr context)
{
	FastMutex::ScopedLock guard(m_mutex);
	m_messages.emplace(context->id(), context);
}

GWMessageContext::Ptr GWContextPoll::remove(const GlobalID &id)
{
	FastMutex::ScopedLock guard(m_mutex);

	GWMessageContext::Ptr context;

	auto it = m_messages.find(id);
	if (it != m_messages.end()) {
		GWTimedContext::Ptr timedContext;
		timedContext = it->second.cast<GWTimedContext>();
		if (!timedContext.isNull())
			timedContext->missingResponseTask()->cancel();

		context = it->second;
		m_messages.erase(it);
	}

	return context;
}

void GWContextPoll::clear()
{
	FastMutex::ScopedLock guard(m_mutex);

	if (m_messages.size()>0) {
		poco_warning(logger(), "clearing "
				+ to_string(m_messages.size())
				+ " of messages still in poll");
	}

	for (auto message : m_messages) {
		GWTimedContext::Ptr timedContext = message.second.cast<GWTimedContext>();
		if (!timedContext.isNull())
			timedContext->missingResponseTask()->cancel();
	}

	m_messages.clear();
}
