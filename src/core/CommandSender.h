#pragma once

#include <Poco/AutoPtr.h>

#include "core/AnswerQueue.h"
#include "core/CommandHandler.h"
#include "util/UnsafePtr.h"

namespace BeeeOn {

class Answer;
class Command;
class CommandDispatcher;

/**
 * Abstract class that provides a generic way to access a
 * CommandDispatcher. The CommandSender MUST be used for
 * dispatching commands.
 */
class CommandSender {
public:
	virtual ~CommandSender();

	/**
	 * Setting raw pointer to avoid circular dependencies when
	 * an implementation of CommandSender also implements CommandHandler
	 * and is registered with the same dispatcher.
	 */
	void setCommandDispatcher(CommandDispatcher *dispatcher);

	/**
	 * Dispatches commands via the configured CommandDispatcher.
	 */
	void dispatch(Poco::AutoPtr<Command> cmd, Poco::AutoPtr<Answer> answer);

	/**
	 * Dispatches commands where the caller does not care about
	 * the answer (there is no sensible reaction possible).
	 */
	void dispatch(Poco::AutoPtr<Command> cmd);

	/**
	 * Provides an implicit answer queue for the sender.
	 */
	AnswerQueue &answerQueue();

private:
	CommandHandler* sendingHandler();

private:
	UnsafePtr<CommandDispatcher> m_commandDispatcher;
	AnswerQueue m_answerQueue;
};

}
