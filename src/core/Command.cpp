#include "core/Command.h"
#include "core/CommandSender.h"
#include "util/ClassInfo.h"

using namespace BeeeOn;
using namespace std;

Command::Command():
	m_sendingHandler(NULL)
{
}

string Command::name() const
{
	return ClassInfo::forPointer(this).name();
}

Command::~Command()
{
}

void Command::setSendingHandler(CommandHandler *sender)
{
	m_sendingHandler = sender;
}

CommandHandler* Command::sendingHandler() const
{
	return m_sendingHandler;
}

string Command::toString() const
{
	return name();
}

Result::Ptr Command::deriveResult(Answer::Ptr answer) const
{
	return new Result(answer);
}
