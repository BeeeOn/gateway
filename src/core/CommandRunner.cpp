#include "core/CommandRunner.h"

using namespace BeeeOn;
using namespace std;

CommandRunner::CommandRunner(Command::Ptr cmd, Answer::Ptr answer,
		Poco::SharedPtr<CommandHandler> handler):
	Task(cmd->name()),
	m_cmd(cmd),
	m_handler(handler),
	m_answer(answer)
{
}

void CommandRunner::runTask()
{
	/*
	 * The TaskManager takes ownership of the Task object
	 * and deletes it when it it finished.
	 */
	m_handler->handle(m_cmd, m_answer);
}
