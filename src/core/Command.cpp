#include "core/Command.h"
#include "core/CommandSender.h"

using namespace BeeeOn;
using namespace std;

Command::Command(const string &commandName):
	m_commandName(commandName)
{
}

string Command::name() const
{
	return m_commandName;
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
