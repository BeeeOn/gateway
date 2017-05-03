#ifndef BEEEON_COMMAND_HANDLER_H
#define BEEEON_COMMAND_HANDLER_H

#include <string>

#include "core/Command.h"
#include "core/Result.h"
#include "core/Answer.h"

namespace BeeeOn {

class Answer;

/*
 * Interface for handling commands. It contains a method to check
 * whether the command is supported and the method to execute
 * such command.
 */
class CommandHandler {
public:
	/*
	 * CommandHandler with specific name of handler.
	 * The name is intended for debugging purposes only.
	 */
	CommandHandler(const std::string &name);

	/*
	 * Name of handler.
	 */
	std::string name() const;

	/*
	 * Returns true if the given command can be handled by this handler.
	 */
	virtual bool accept(const Command::Ptr cmd) = 0;

	/*
	 * This method is likely to be called concurrently. It must be
	 * implemented in a thread-safe way. It must create the Result
	 * and it must add the Answer.
	 *
	 * For example: Mutex::ScopedLock lock(m_mutex);
	 */
	virtual void handle(Command::Ptr cmd, Answer::Ptr answer) = 0;

private:
	std::string m_name;
};

}

#endif
