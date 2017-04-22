#include <vector>

#include <Poco/Observer.h>

#include "core/Answer.h"
#include "core/AnswerQueue.h"
#include "core/CommandRunner.h"

using namespace BeeeOn;
using namespace Poco;

Answer::Answer(AnswerQueue &answerQueue):
	m_answerQueue(answerQueue),
	m_dirty(0),
	m_taskManager(Poco::ThreadPool::defaultPool())
{
	answerQueue.add(this);
	installObservers();
}

void Answer::setDirty(bool dirty)
{
	FastMutex::ScopedLock guard(lock());
	setDirtyUnlocked(dirty);
}

void Answer::setDirtyUnlocked(bool dirty)
{
	assureLocked();
	m_dirty = dirty;
}

bool Answer::isDirty() const
{
	FastMutex::ScopedLock guard(lock());
	return isDirtyUnlocked();
}

bool Answer::isDirtyUnlocked() const
{
	assureLocked();
	return (bool) m_dirty;
}

Event &Answer::event()
{
	return m_answerQueue.event();
}

FastMutex &Answer::lock() const
{
	return m_lock;
}

bool Answer::isPending() const
{
	FastMutex::ScopedLock guard(m_lock);
	return isPendingUnlocked();
}

bool Answer::isPendingUnlocked() const
{
	assureLocked();

	if ((unsigned long) m_commands != m_resultList.size())
		return true;

	if (m_commands == 0)
		return false;

	for (auto &result : m_resultList) {
		if (result->statusUnlocked() == Result::PENDING)
			return true;
	}

	return false;
}

void Answer::runCommands()
{
	assureLocked();

	for (auto item : m_commandList) {
		m_taskManager.start(item);
	}

	m_commandList.clear();
}

void Answer::installObservers()
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

bool Answer::isEmpty() const
{
	return m_commands == 0;
}

unsigned long Answer::resultsCount() const
{
	FastMutex::ScopedLock guard(m_lock);
	return m_resultList.size();
}

void Answer::addCommand(Poco::SharedPtr<CommandHandler> handler,
	Command::Ptr cmd, Answer::Ptr answer)
{
	FastMutex::ScopedLock guard(m_lock);
	m_commandList.push_back(new CommandRunner(cmd, answer, handler));
	m_commands++;
}

void Answer::addResult(Result *result)
{
	FastMutex::ScopedLock guard(m_lock);
	m_resultList.push_back(AutoPtr<Result>(result, true));
}

int Answer::commandsCount() const
{
	FastMutex::ScopedLock guard(m_lock);
	return m_commands;
}

void Answer::assureLocked() const
{
	if (lock().tryLock()) {
		lock().unlock();
		throw IllegalStateException(
			"modifying unlocked Answer");
	}
}

void Answer::notifyUpdated()
{
	assureLocked();
	setDirtyUnlocked(true);
	event().set();
}

Result::Ptr Answer::at(size_t position)
{
	FastMutex::ScopedLock guard(m_lock);
	return atUnlocked(position);
}

Result::Ptr Answer::atUnlocked(size_t position)
{
	assureLocked();
	return m_resultList.at(position);
}

std::vector<Result::Ptr>::iterator Answer::begin()
{
	assureLocked();
	return m_resultList.begin();
}

std::vector<Result::Ptr>::iterator Answer::end()
{
	assureLocked();
	return m_resultList.end();
}
