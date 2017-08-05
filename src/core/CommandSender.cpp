#include "core/CommandDispatcher.h"
#include "core/CommandSender.h"

using namespace BeeeOn;
using namespace Poco;

CommandSender::~CommandSender()
{
}

void CommandSender::setCommandDispatcher(
	SharedPtr<CommandDispatcher> dispatcher)
{
	m_commandDispatcher = dispatcher;
}

void CommandSender::dispatch(AutoPtr<Command> cmd, AutoPtr<Answer> answer)
{
	// If the sender does not implement CommandHandler, the handler
	// is NULL. Otherwise, we set ourself to prevent "dispatch to
	// itself" issue.
	CommandHandler *handler = dynamic_cast<CommandHandler *>(this);
	cmd->setSendingHandler(handler);

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

SharedPtr<CommandDispatcher> CommandSender::commandDispatcher() const
{
	return m_commandDispatcher;
}
