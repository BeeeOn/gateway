#include <Poco/Exception.h>

#include "core/CommandRunner.h"

using namespace BeeeOn;
using namespace Poco;
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
	try {
		/*
		 * The TaskManager takes ownership of the Task object
		 * and deletes it when it it finished.
		 */
		m_handler->handle(m_cmd, m_answer);
	}
	catch (Exception &ex) {
		logger().log(ex);
	}
	catch (exception &ex) {
		poco_critical(logger(), ex.what());
	}
	catch(...) {
		poco_critical(logger(), "unknown error");
	}
}
