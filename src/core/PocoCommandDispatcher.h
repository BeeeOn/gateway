#ifndef BEEEON_POCO_COMMAND_DISPATCHER_H
#define BEEEON_POCO_COMMAND_DISPATCHER_H

#include <list>

#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>

#include "core/CommandHandler.h"
#include "core/CommandDispatcher.h"
#include "core/Command.h"
#include "util/Loggable.h"

namespace BeeeOn {

class Answer;
class Command;

/*
 * The CommandDispatcher the given command to the target handler and
 * contain registered command handlers.
 */
class PocoCommandDispatcher : public CommandDispatcher, public Loggable {
public:
	/*
	 * The operation might be asynchronous. The result of the command can come
	 * later. The given instance of result must ensure that the result
	 * will be set to Result::SUCCESS or Result::ERROR after the execution of
	 * CommandHandler::handle().
	 */
	void dispatch(Command::Ptr cmd, Answer::Ptr answer) override;

private:
	Poco::FastMutex m_mutex;
};

}

#endif
