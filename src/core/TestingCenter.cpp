#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/StringTokenizer.h>

#include "di/Injectable.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "commands/ServerDeviceListCommand.h"
#include "commands/ServerLastValueCommand.h"
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

	if (args[1] == "unpair") {
		assureArgs(context, 3, "command unpair");
		return new DeviceUnpairCommand(DeviceID::parse(args[2]));
	}
	else if (args[1] == "set-value") {
		assureArgs(context, 5, "command set-value");

		Timespan timeout(0);
		if (args.size() >= 6)
			timeout = Timespan(NumberParser::parse(args[5]) * Timespan::MILLISECONDS);

		return new DeviceSetValueCommand(
			DeviceID::parse(args[2]),
			ModuleID::parse(args[3]),
			NumberParser::parseFloat(args[4]),
			timeout
		);
	}
	else if (args[1] == "listen") {
		assureArgs(context, 2, "command listen");

		Timespan duration(5 * Timespan::SECONDS);
		if (args.size() > 2)
			duration = Timespan(NumberParser::parse(args[2]) * Timespan::SECONDS);

		return new GatewayListenCommand(duration);
	}
	else if (args[1] == "list-devices") {
		assureArgs(context, 3, "command list-devices");

		return new ServerDeviceListCommand(
			DevicePrefix::parse(args[2])
		);
	}
	else if (args[1] == "last-value") {
		assureArgs(context, 4, "command last-value");

		return new ServerLastValueCommand(
			DeviceID::parse(args[2]),
			ModuleID::parse(args[3])
		);
	}

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
		console.print("  unpair <device-id>");
		console.print("  set-value <device-id> <module-id> <value> [<timeout>]");
		console.print("  listen [<timeout>]");
		console.print("  list-devices <device-prefix>");
		console.print("  last-value <device-id> <module-id>");
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

static void waitQueueAction(TestingCenter::ActionContext &context)
{
	ConsoleSession &console = context.console;
	auto &args = context.args;

	if (args.size() > 1 && args[1] == "help") {
		console.print("usage: wait-queue [<timeout>]");
		return;
	}

	Timespan timeout(0);
	if (args.size() > 1)
		timeout = Timespan(NumberParser::parse(args[1]) * Timespan::MILLISECONDS);

	list<Answer::Ptr> dirtyList;
	context.queue.wait(timeout, dirtyList);

	for (auto dirty : dirtyList) {
		FastMutex::ScopedLock guard(dirty->lock());

		console.print(reportAnswer(dirty, guard));

		if (!dirty->isPendingUnlocked())
			context.queue.remove(dirty);
	}
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

static void deviceAction(TestingCenter::ActionContext &context)
{
	ConsoleSession &console = context.console;
	Command::Ptr command;

	if (context.args.size() <= 1) {
		console.print("missing arguments for action 'device'");
		return;
	}

	if (context.args[1] == "help") {
		console.print("usage: device <action> [<args>...]");
		console.print("actions:");
		return;
	}
}

TestingCenter::TestingCenter():
	m_stop(0)
{
	registerAction("echo", echoAction, "echo arguments to output separated by space");
	registerAction("command", commandAction, "dispatch a command into the system");
	registerAction("wait-queue", waitQueueAction, "wait for new command answers");
	registerAction("device", deviceAction, "simulate device in server database");
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
	ActionContext context {session, m_queue, m_dispatcher,
			m_devices, m_mutex, args};
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
