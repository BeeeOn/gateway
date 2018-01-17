#include <Poco/Exception.h>

#include "core/Answer.h"
#include "core/PocoCommandDispatcher.h"
#include "di/Injectable.h"
#include "core/PocoAnswerImpl.h"

BEEEON_OBJECT_BEGIN(BeeeOn, PocoCommandDispatcher)
BEEEON_OBJECT_CASTABLE(CommandDispatcher)
BEEEON_OBJECT_REF("handlers", &PocoCommandDispatcher::registerHandler)
BEEEON_OBJECT_REF("listeners", &PocoCommandDispatcher::registerListener)
BEEEON_OBJECT_REF("eventsExecutor", &PocoCommandDispatcher::setEventsExecutor)
BEEEON_OBJECT_END(BeeeOn, PocoCommandDispatcher)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

void PocoCommandDispatcher::dispatchImpl(Command::Ptr cmd, Answer::Ptr answer)
{
	Poco::FastMutex::ScopedLock guard(m_mutex);

	PocoAnswerImpl::Ptr impl = new PocoAnswerImpl;
	injectImpl(answer, impl);

	for (auto item : m_commandHandlers) {
		// avoid "dispatch to itself" issue
		if (item.get() == cmd->sendingHandler())
			continue;

		try {
			if (!item->accept(cmd))
				continue;

			impl->addTask(item, cmd, answer);
		}
		catch (Exception &ex) {
			logger().log(ex);
		}
		catch (exception &ex) {
			poco_critical(logger(), ex.what());
		}
		catch(...) {
			poco_critical(logger(), "unknown error");
		}
	}

	answer->setHandlersCount(impl->tasks());

	Answer::ScopedLock lock(*answer);
	if (!answer->isPending()) {
		answer->notifyUpdated();
		return;
	}

	impl->runTasks();
}
