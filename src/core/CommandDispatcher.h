#ifndef BEEEON_COMMAND_DISPATCHER_H
#define BEEEON_COMMAND_DISPATCHER_H

#include <Poco/SharedPtr.h>

#include "core/CommandDispatcherListener.h"
#include "core/CommandHandler.h"
#include "util/EventSource.h"
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

	void registerListener(CommandDispatcherListener::Ptr listener);
	void setExecutor(Poco::SharedPtr<AsyncExecutor> executor);

protected:
	void injectImpl(Answer::Ptr answer, Poco::SharedPtr<AnswerImpl> impl);

	/**
	 * This operation is suppose to be called, when dispatch has occured to
	 * notify all registered listeners about this event.
	 */
	void notifyDispatch(Command::Ptr);

protected:
	std::list<Poco::SharedPtr<CommandHandler>> m_commandHandlers;

private:
	EventSource<CommandDispatcherListener> m_eventSource;
};

}

#endif
