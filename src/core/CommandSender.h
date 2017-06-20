#ifndef BEEEON_COMMAND_SENDER_H
#define BEEEON_COMMAND_SENDER_H

#include <Poco/AutoPtr.h>
#include <Poco/SharedPtr.h>

#include "core/AnswerQueue.h"
#include "core/CommandHandler.h"

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

	void setCommandDispatcher(Poco::SharedPtr<CommandDispatcher> dispatcher);
	Poco::SharedPtr<CommandDispatcher> commandDispatcher() const;

	/**
	 * Dispatches commands via the configured CommandDispatcher.
	 */
	void dispatch(Poco::AutoPtr<Command> cmd, Poco::AutoPtr<Answer> answer);

	/**
	 * Provides an implicit answer queue for the sender.
	 */
	AnswerQueue &answerQueue();

private:
	Poco::SharedPtr<CommandDispatcher> m_commandDispatcher;
	AnswerQueue m_answerQueue;
};

}

#endif
