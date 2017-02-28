#include "core/Command.h"

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
