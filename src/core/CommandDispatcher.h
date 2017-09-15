#ifndef BEEEON_COMMAND_DISPATCHER_H
#define BEEEON_COMMAND_DISPATCHER_H

#include <Poco/SharedPtr.h>

#include "core/CommandHandler.h"
#include "util/Loggable.h"

namespace BeeeOn {

class CommandDispatcher : protected Loggable {
public:
	virtual ~CommandDispatcher();

	/*
	 * Register a command handler for command dispatching.
	 */
	void registerHandler(Poco::SharedPtr<CommandHandler> handler);

	/*
	 * The operation might be asynchronous. The result of the command can come
	 * later. The given instance of result must ensure that the result
	 * will be set to Result::SUCCESS or Result::ERROR after the execution of
	 * CommandHandler::handle().
	 */
	virtual void dispatch(Command::Ptr cmd, Answer::Ptr answer) = 0;

protected:
	void injectImpl(Answer::Ptr answer, Poco::SharedPtr<AnswerImpl> impl);

protected:
	std::list<Poco::SharedPtr<CommandHandler>> m_commandHandlers;
};

}

#endif
