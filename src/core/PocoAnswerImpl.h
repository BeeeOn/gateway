#ifndef BEEEON_POCO_ANSWER_IMPL_H
#define BEEEON_POCO_ANSWER_IMPL_H

#include <Poco/AutoPtr.h>
#include <Poco/RefCountedObject.h>
#include <Poco/SharedPtr.h>

#include "core/Answer.h"
#include "core/Command.h"

namespace BeeeOn {

class CommandHandler;

class PocoAnswerImpl {
public:
	typedef Poco::SharedPtr<PocoAnswerImpl> Ptr;

	PocoAnswerImpl();

	/**
	 * Run all commands from the Answer.
	 */
	void runTasks();

	/**
	 * Registers an observer with the NotificationCenter.
	 */
	void installObservers();

	/**
	 * @return number of tasks to be executed
	 */
	unsigned long tasks() const;

	/**
	 * Creates a new Poco::Task for the given arguments. The task
	 * implements calling the associated handler in a separate thread.
	 */
	void addTask(Poco::SharedPtr<CommandHandler> handler,
		Command::Ptr cmd, Answer::Ptr answer);

private:
	Poco::TaskManager m_taskManager;
	CommandProgressHandler m_commandProgressHandler;
	std::vector<Poco::AutoPtr<Poco::Task>> m_taskList;
};

}

#endif
