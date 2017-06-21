#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/StringTokenizer.h>

#include "di/Injectable.h"
#include "commands/DeviceSetValueCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "commands/NewDeviceCommand.h"
#include "commands/ServerDeviceListCommand.h"
#include "commands/ServerDeviceListResult.h"
#include "commands/ServerLastValueCommand.h"
#include "commands/ServerLastValueResult.h"
#include "core/Command.h"
#include "core/TestingCenter.h"
#include "util/ArgsParser.h"

BEEEON_OBJECT_BEGIN(BeeeOn, TestingCenter)
BEEEON_OBJECT_CASTABLE(CommandHandler)
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
	line += to_string(p->handlersCountUnlocked());

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

		if (args[2].substr(0, 2) == "0x") {
			return new ServerDeviceListCommand(
				DevicePrefix::fromRaw(NumberParser::parseHex(args[2]))
			);
		}
		else {
			return new ServerDeviceListCommand(
				DevicePrefix::parse(args[2])
			);
		}
	}
	else if (args[1] == "last-value") {
		assureArgs(context, 4, "command last-value");

		return new ServerLastValueCommand(
			DeviceID::parse(args[2]),
			ModuleID::parse(args[3])
		);
	}
	else if (args[1] == "new-device") {
		assureArgs(context, 6, "command new-device");

		list<ModuleType> dataTypes;
		for (unsigned int i = 6; i < context.args.size(); i++) {
			set<ModuleType::Attribute> attributes;

			StringTokenizer tokens(args[i], ",",
				StringTokenizer::TOK_IGNORE_EMPTY | StringTokenizer::TOK_TRIM);

			if (tokens.count() > 1) {
				for (unsigned int k = 1; k < tokens.count(); k++)
					attributes.insert(ModuleType::Attribute::parse(tokens[k]));
			}

			dataTypes.push_back(
				ModuleType(
					ModuleType::Type::parse(tokens[0]),
					attributes
				)
			);
		}

		return new NewDeviceCommand(
			DeviceID::parse(args[2]),
			args[3],
			args[4],
			dataTypes,
			NumberParser::parse(args[5])
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
		console.print("  new-device <device-id> <vendor> <product-name> <refresh-time> "
			"[<type>,[<attribute>]...]...");
		return;
	}

	command = parseCommand(context);

	if (command.isNull()) {
		console.print("unrecognized command: " + context.args[1]);
		return;
	}

	Answer::Ptr answer(new Answer(context.sender.answerQueue()));

	context.sender.dispatch(command, answer);

	FastMutex::ScopedLock guard(answer->lock());
	console.print(reportAnswer(answer, guard));

	if (!answer->isPendingUnlocked())
		context.sender.answerQueue().remove(answer);
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
	context.sender.answerQueue().wait(timeout, dirtyList);

	for (auto dirty : dirtyList) {
		FastMutex::ScopedLock guard(dirty->lock());

		console.print(reportAnswer(dirty, guard));

		if (!dirty->isPendingUnlocked())
			context.sender.answerQueue().remove(dirty);
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
		console.print("  create <device-id> [<module-value>...]");
		console.print("  update <device-id> <module-id> <module-value>");
		console.print("  list");
		console.print("  delete <device-id>");
		return;
	}
	else if (context.args[1] == "create") {
		assureArgs(context, 3, "device create");

		DeviceID deviceID(DeviceID::parse(context.args[2]));
		TestingCenter::DeviceData deviceData;

		for (unsigned i = 3; i < context.args.size(); i++)
			deviceData[i - 3] = NumberParser::parseFloat(context.args[i]);

		ScopedLock<Mutex> guard(context.mutex);
		context.devices[deviceID] = deviceData;

		context.console.print(deviceID.toString() + " created");
	}
	else if (context.args[1] == "update") {
		assureArgs(context, 5, "device update");

		DeviceID deviceID = DeviceID::parse(context.args[2]);
		ModuleID moduleId = ModuleID::parse(context.args[3]);
		double value = NumberParser::parseFloat(context.args[4]);

		ScopedLock<Mutex> guard(context.mutex);
		context.devices[deviceID][moduleId] = value;

		context.console.print(deviceID.toString() + " updated");
	}
	else if (context.args[1] == "list") {
		assureArgs(context, 2, "device list");

		ScopedLock<Mutex> guard(context.mutex);
		for (auto &deviceIt: context.devices){
			console.print(deviceIt.first.toString());

			for (auto &moduleIt: deviceIt.second) {
				console.print(moduleIt.first.toString()
						+ ": " + to_string(moduleIt.second));
			}
		}
	}
	else if (context.args[1] == "delete") {
		assureArgs(context, 3, "device delete");

		DeviceID deviceID(DeviceID::parse(context.args[2]));

		ScopedLock<Mutex> guard(context.mutex);
		context.devices.erase(deviceID);

		context.console.print(deviceID.toString() + " deleted");
	}
}

TestingCenter::TestingCenter():
	CommandHandler("TestingCenter"),
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
	ArgsParser parser;
	vector<string> args;

	try {
		args = parser.parse(line);
	}
	catch (const SyntaxException &e) {
		logger().log(e, __FILE__, __LINE__);
		session.print("error: " + e.message());
		return;
	}

	auto it = m_action.find(args[0]);
	if (it == m_action.end()) {
		session.print("no such action defined");
		return;
	}

	ActionContext context {session, m_devices, m_mutex, *this, args};
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

bool TestingCenter::accept(const Command::Ptr cmd)
{
	if (cmd->is<ServerLastValueCommand>()) {
		auto command = cmd.cast<ServerLastValueCommand>();
		ScopedLock<Mutex> guard(m_mutex);
		return m_devices.find(command->deviceID()) != m_devices.end();
	}

	return cmd->is<ServerDeviceListCommand>();
}

void TestingCenter::handle(Command::Ptr cmd, Answer::Ptr answer)
{
	if (cmd->is<ServerDeviceListCommand>()) {
		DevicePrefix prefix = cmd.cast<ServerDeviceListCommand>()->devicePrefix();
		ServerDeviceListResult::Ptr result = new ServerDeviceListResult(answer);

		vector<DeviceID> devices;

		ScopedLock<Mutex> guard(m_mutex);
		for (auto &it : m_devices) {
			if (it.first.prefix() == prefix)
				devices.push_back(it.first);
		}

		result->setDeviceList(devices);
		result->setStatus(Result::SUCCESS);
	}
	else if (cmd->is<ServerLastValueCommand>()) {
		ServerLastValueCommand::Ptr command = cmd.cast<ServerLastValueCommand>();
		ServerLastValueResult::Ptr result = new ServerLastValueResult(answer);

		ScopedLock<Mutex> guard(m_mutex);
		auto deviceIt = m_devices.find(command->deviceID());
		if (deviceIt == m_devices.end()) {
			result->setStatus(Result::FAILED);
			return;
		}

		auto moduleIt = deviceIt->second.find(command->moduleID());
		if (moduleIt == deviceIt->second.end()) {
			result->setStatus(Result::FAILED);
			return;
		}

		result->setValue(moduleIt->second);
		result->setStatus(Result::SUCCESS);
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
