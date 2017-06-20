#include <Poco/Exception.h>

#include "core/Answer.h"
#include "core/PocoCommandDispatcher.h"
#include "di/Injectable.h"

BEEEON_OBJECT_BEGIN(BeeeOn, PocoCommandDispatcher)
BEEEON_OBJECT_CASTABLE(PocoCommandDispatcher)
BEEEON_OBJECT_REF("registerHandler", &PocoCommandDispatcher::registerHandler)
BEEEON_OBJECT_END(BeeeOn, PocoCommandDispatcher)

using namespace BeeeOn;

void PocoCommandDispatcher::registerHandler(Poco::SharedPtr<CommandHandler> handler)
{
	for (auto &item : m_commandHandlers) {
		if (item.get() == handler.get()) {
			throw Poco::ExistsException("handler " + handler->name()
				+ " has been registered");
		}
	}

	m_commandHandlers.push_back(handler);
}

void PocoCommandDispatcher::dispatch(Command::Ptr cmd, Answer::Ptr answer)
{
	Poco::FastMutex::ScopedLock guard(m_mutex);

	for (auto item : m_commandHandlers) {
		if (!item->accept(cmd))
			continue;

		answer->addCommand(item, cmd, answer);
	}

	Poco::FastMutex::ScopedLock lock(answer->lock());
	if (!answer->isPendingUnlocked()) {
		answer->notifyUpdated();
		return;
	}

	answer->runCommands();
}
