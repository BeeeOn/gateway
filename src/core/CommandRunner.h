#ifndef BEEEON_COMMAND_RUNNER_H
#define BEEEON_COMMAND_RUNNER_H

#include <string>

#include <Poco/SharedPtr.h>
#include <Poco/Task.h>

#include "core/CommandHandler.h"
#include "core/Answer.h"
#include "util/Loggable.h"

namespace BeeeOn {

class Answer;
class Command;
class CommandHandler;

/*
 * Execute the CommandHandler::handle() method in a separate thread
 * via the Poco::Task class. Thus, the command handling is
 * always non-blocking.
 */
class CommandRunner : public Poco::Task, public Loggable {
public:
	CommandRunner(Command::Ptr cmd, Answer::Ptr answer,
		Poco::SharedPtr<CommandHandler> handler);

	/*
	 * Executes method CommandHandler::handle in a separate thread.
	 */
	void runTask();

private:
	Command::Ptr m_cmd;
	Poco::SharedPtr<CommandHandler> m_handler;
	Answer::Ptr m_answer;
};

}

#endif
