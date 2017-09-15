#include <Poco/Logger.h>

#include "core/CommandDispatcher.h"

using namespace BeeeOn;
using namespace Poco;

CommandDispatcher::~CommandDispatcher()
{
}

void CommandDispatcher::registerHandler(SharedPtr<CommandHandler> handler)
{
	for (auto &item : m_commandHandlers) {
		if (item == handler)
			throw Poco::ExistsException("duplicate handler detected");
	}

	m_commandHandlers.push_back(handler);
}

void CommandDispatcher::injectImpl(Answer::Ptr answer, SharedPtr<AnswerImpl> impl)
{
	answer->installImpl(impl);
}
