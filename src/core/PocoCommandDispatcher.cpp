#include <Poco/Exception.h>

#include "core/Answer.h"
#include "core/PocoCommandDispatcher.h"
#include "di/Injectable.h"
#include "core/PocoAnswerImpl.h"

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

	PocoAnswerImpl::Ptr impl = new PocoAnswerImpl;
	injectImpl(answer, impl);

	for (auto item : m_commandHandlers) {
		if (!item->accept(cmd))
			continue;

		impl->addTask(item, cmd, answer);
	}

	answer->setHandlersCount(impl->tasks());

	Poco::FastMutex::ScopedLock lock(answer->lock());
	if (!answer->isPendingUnlocked()) {
		answer->notifyUpdated();
		return;
	}

	impl->runTasks();
}

void PocoCommandDispatcher::injectImpl(Answer::Ptr answer, Poco::SharedPtr<AnswerImpl> impl)
{
	answer->installImpl(impl);
}
