#include <Poco/Observer.h>
#include <Poco/ThreadPool.h>

#include "core/CommandRunner.h"
#include "core/PocoAnswerImpl.h"

using namespace BeeeOn;
using namespace Poco;


PocoAnswerImpl::PocoAnswerImpl():
	m_taskManager(Poco::ThreadPool::defaultPool())
{
}

void PocoAnswerImpl::runTasks()
{
	for (auto &item : m_taskList)
		m_taskManager.start(item);
}

void PocoAnswerImpl::installObservers()
{
	m_taskManager.addObserver(
		Observer<CommandProgressHandler, Poco::TaskFinishedNotification>
			(m_commandProgressHandler, &CommandProgressHandler::onFinished)
	);

	m_taskManager.addObserver(
		Observer<CommandProgressHandler, Poco::TaskFailedNotification>
			(m_commandProgressHandler, &CommandProgressHandler::onFailed)
	);
	m_taskManager.addObserver(
		Observer<CommandProgressHandler, Poco::TaskStartedNotification>
			(m_commandProgressHandler, &CommandProgressHandler::onStarted)
	);

	m_taskManager.addObserver(
		Observer<CommandProgressHandler, Poco::TaskCancelledNotification>
			(m_commandProgressHandler, &CommandProgressHandler::onCancel)
	);
}

void PocoAnswerImpl::addTask(Poco::SharedPtr<CommandHandler> handler, Command::Ptr cmd, Answer::Ptr answer)
{
	// Increment reference counter of the runner. Thus, TaskManager
	// would not destroy it and the destruction is invoked in the
	// destructor of PocoAnswerImpl.
	AutoPtr<CommandRunner> ptr(new CommandRunner(cmd, answer, handler), true);
	m_taskList.push_back(ptr);
}

unsigned long PocoAnswerImpl::tasks() const
{
	return m_taskList.size();
}
