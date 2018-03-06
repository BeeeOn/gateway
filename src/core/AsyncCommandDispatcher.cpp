#include <Poco/Logger.h>

#include "core/AsyncCommandDispatcher.h"
#include "di/Injectable.h"

BEEEON_OBJECT_BEGIN(BeeeOn, AsyncCommandDispatcher)
BEEEON_OBJECT_CASTABLE(CommandDispatcher)
BEEEON_OBJECT_PROPERTY("handlers", &AsyncCommandDispatcher::registerHandler)
BEEEON_OBJECT_PROPERTY("listeners", &AsyncCommandDispatcher::registerListener)
BEEEON_OBJECT_PROPERTY("commandsExecutor", &AsyncCommandDispatcher::setCommandsExecutor)
BEEEON_OBJECT_PROPERTY("eventsExecutor", &AsyncCommandDispatcher::setEventsExecutor)
BEEEON_OBJECT_END(BeeeOn, AsyncCommandDispatcher)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

void AsyncCommandDispatcher::setCommandsExecutor(ParallelExecutor::Ptr executor)
{
	m_commandsExecutor = executor;
}

void AsyncCommandDispatcher::dispatchImpl(
		Command::Ptr cmd, Answer::Ptr answer)
{
	list<SharedPtr<CommandHandler>> handlers;

	for (auto handler : m_commandHandlers) {
		if (handler.get() == cmd->sendingHandler())
			continue;

		try {
			if (!handler->accept(cmd))
				continue;

			handlers.emplace_back(handler);
		}
		BEEEON_CATCH_CHAIN(logger())
	}

	answer->setHandlersCount(handlers.size());

	Answer::ScopedLock guard(*answer);
	if (!answer->isPending()) {
		answer->notifyUpdated();
		return;
	}

	for (auto handler : handlers) {
		m_commandsExecutor->invoke([handler, cmd, answer]() mutable {
			Logger &logger = Loggable::forClass(typeid(*handler));

			try {
				handler->handle(cmd, answer);
			}
			BEEEON_CATCH_CHAIN(logger)
		});
	}
}
