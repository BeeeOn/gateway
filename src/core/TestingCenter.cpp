#include <Poco/NumberFormatter.h>
#include <Poco/StringTokenizer.h>

#include "di/Injectable.h"
#include "core/Command.h"
#include "core/TestingCenter.h"

BEEEON_OBJECT_BEGIN(BeeeOn, TestingCenter)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_REF("commandDispatcher", &TestingCenter::setCommandDispatcher)
BEEEON_OBJECT_REF("console", &TestingCenter::setConsole)
BEEEON_OBJECT_END(BeeeOn, TestingCenter)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

static string identifyAnswer(const Answer::Ptr p)
{
	return NumberFormatter::formatHex(reinterpret_cast<size_t>(p.get()), true);
}

static string reportAnswer(Answer::Ptr p, FastMutex::ScopedLock &)
{
	string line(identifyAnswer(p));

	line += p->isPendingUnlocked()? " PENDING " : " DONE ";
	line += to_string(p->resultsCountUnlocked());
	line += "/";
	line += to_string(p->commandsCountUnlocked());

	return line;
}

static void assureArgs(
		const TestingCenter::ActionContext &context,
		size_t expectedCount,
		const string &command)
{
	if (context.args.size() < expectedCount) {
		throw InvalidArgumentException(
			"missing arguments for action '" + command + "'");
	}
}

static Command::Ptr parseCommand(TestingCenter::ActionContext &context)
{
	auto &args = context.args;

	return NULL;
}

static void commandAction(TestingCenter::ActionContext &context)
{
	ConsoleSession &console = context.console;
	Command::Ptr command;

	if (context.args.size() <= 1) {
		console.print("missing arguments for action 'command'");
		return;
	}

	if (context.args[1] == "help") {
		console.print("usage: command <name> [<args>...]");
		console.print("names:");
		return;
	}

	command = parseCommand(context);

	if (command.isNull()) {
		console.print("unrecognized command: " + context.args[0]);
		return;
	}

	Answer::Ptr answer(new Answer(context.queue));

	context.dispatcher->dispatch(command, answer);

	FastMutex::ScopedLock guard(answer->lock());
	console.print(reportAnswer(answer, guard));

	if (!answer->isPendingUnlocked())
		context.queue.remove(answer);
}


/**
 * Echo the space-separated arguments to the console.
 */
static void echoAction(TestingCenter::ActionContext &context)
{
	ConsoleSession &console = context.console;
	auto &args = context.args;

	for (size_t i = 1; i < args.size(); ++i) {
		if (i > 1)
			console.print(" ", false);

		console.print(args[i], false);
	}

	console.print("");
}

TestingCenter::TestingCenter():
	m_stop(0)
{
	registerAction("echo", echoAction, "echo arguments to output separated by space");
	registerAction("command", commandAction, "dispatch a command into the system");
}

void TestingCenter::registerAction(
		const string &name,
		const Action action,
		const string &description)
{
	ActionRecord record = {description, action};
	m_action.emplace(name, record);
}

void TestingCenter::setCommandDispatcher(SharedPtr<CommandDispatcher> dispatcher)
{
	m_dispatcher = dispatcher;
}

SharedPtr<CommandDispatcher> TestingCenter::commandDispatcher() const
{
	return m_dispatcher;
}

void TestingCenter::setConsole(SharedPtr<Console> console)
{
	m_console = console;
}

SharedPtr<Console> TestingCenter::console() const
{
	return m_console;
}

void TestingCenter::printHelp(ConsoleSession &session)
{
	session.print("Gateway Testing Center");
	session.print("Commands:");
	session.print("  help - print this help");
	session.print("  exit - exit the console session");

	for (const auto &action : m_action) {
		const ActionRecord &record = action.second;
		session.print("  " + action.first + " - " + record.description);
	}
}

void TestingCenter::processLine(ConsoleSession &session, const string &line)
{
	StringTokenizer action(line, " ",
		StringTokenizer::TOK_IGNORE_EMPTY | StringTokenizer::TOK_TRIM);

	auto it = m_action.find(action[0]);
	if (it == m_action.end()) {
		session.print("no such action defined");
		return;
	}

	vector<string> args(action.begin(), action.end());
	ActionContext context {session, m_queue, m_dispatcher, args};
	Action f = it->second.action;

	try {
		f(context);
	}
	catch (const Exception &e) {
		logger().log(e, __FILE__, __LINE__);
		session.print("error: " + e.displayText());
	}
	catch (const exception &e) {
		logger().critical(e.what(), __FILE__, __LINE__);
		session.print("error: " + string(e.what()));
		throw;
	}
	catch (...) {
		logger().critical("unknown error", __FILE__, __LINE__);
		session.print("unknown error");
		throw;
	}
}

void TestingCenter::run()
{
	logger().information("Starting Gateway Testing Center",
			__FILE__, __LINE__);
	logger().critical("TESTING CENTER IS NOT INTENDED FOR PRODUCTION",
			__FILE__, __LINE__);

	while (!m_stop) {
		ConsoleSession session(*m_console);

		while (!m_stop && !session.eof()) {
			const string line = session.readLine();

			if (line.empty())
				continue;

			if (line == "help") {
				printHelp(session);
				continue;
			}

			if (line == "exit")
				break;

			try {
				processLine(session, line);
			} catch (...) {
				session.print("closing session");
				break;
			}
		}
	}

	logger().information("Closing Gateway Testing Center");
}

void TestingCenter::stop()
{
	m_stop = 1;
}
