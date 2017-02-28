#include "core/CommandHandler.h"

using namespace BeeeOn;
using namespace std;

CommandHandler::CommandHandler(const string &name):
	m_name(name)
{
}

string CommandHandler::name() const
{
	return m_name;
}
