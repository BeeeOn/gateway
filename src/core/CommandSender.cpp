#include "core/CommandDispatcher.h"
#include "core/CommandSender.h"

using namespace BeeeOn;
using namespace Poco;

CommandSender::~CommandSender()
{
}

void CommandSender::setCommandDispatcher(CommandDispatcher *dispatcher)
{
	m_commandDispatcher = dispatcher;
}

void CommandSender::dispatch(AutoPtr<Command> cmd, AutoPtr<Answer> answer)
{
	cmd->setSendingHandler(sendingHandler());
	m_commandDispatcher->dispatch(cmd, answer);
}

void CommandSender::dispatch(AutoPtr<Command> cmd)
{
	Answer::Ptr answer = new Answer(answerQueue());
	dispatch(cmd, answer);

	answer->waitNotPending(-1);
}

AnswerQueue &CommandSender::answerQueue()
{
	return m_answerQueue;
}

CommandHandler* CommandSender::sendingHandler()
{
	// If the sender does not implement CommandHandler, the handler
	// is NULL. Otherwise, we set ourself to prevent "dispatch to
	// itself" issue.
	return dynamic_cast<CommandHandler *>(this);
}
