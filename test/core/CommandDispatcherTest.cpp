#include <cppunit/extensions/HelperMacros.h>

#include <Poco/Thread.h>
#include <Poco/ThreadPool.h>

#include "core/AnswerQueue.h"
#include "core/AsyncCommandDispatcher.h"
#include "core/CommandSender.h"
#include "core/Result.h"
#include "model/DeviceID.h"
#include "util/ParallelExecutor.h"

namespace BeeeOn {

class CommandDispatcherTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(CommandDispatcherTest);
	CPPUNIT_TEST(testSupportedCommand);
	CPPUNIT_TEST(testUnsupportedCommand);
	CPPUNIT_TEST(testCommandSender);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp();
	void tearDown();

	void testSupportedCommand();
	void testUnsupportedCommand();
	void testCommandSender();

private:
	ParallelExecutor::Ptr m_executor;
	Poco::Thread m_executorThread;
};

CPPUNIT_TEST_SUITE_REGISTRATION(CommandDispatcherTest);

class FakeCommand : public Command {
public:
	typedef Poco::AutoPtr<FakeCommand> Ptr;

	FakeCommand(const DeviceID &deviceID):
		m_deviceID(deviceID)
	{
	}

	DeviceID deviceID() const
	{
		return m_deviceID;
	}

	CommandHandler* sendingHandler() const
	{
		return Command::sendingHandler();
	}

private:
	DeviceID m_deviceID;
};

/*
 * The handler supports the FakeCommand command. The task execution
 * is set to 20 ms and starts before the FakeHandler2.
 */
class FakeHandler1 : public CommandHandler {
public:
	FakeHandler1(const DeviceID &deviceID):
		m_deviceID(deviceID)
	{
	}

	bool accept(Command::Ptr cmd) override
	{
		if (cmd->is<FakeCommand>()) {
			return m_deviceID == cmd.cast<FakeCommand>()->deviceID();
		}

		return false;
	}

	void handle(Command::Ptr cmd, Answer::Ptr answer) override
	{
		if (cmd->is<FakeCommand>()) {
			Poco::Thread::sleep(20);

			Result::Ptr result = new Result(answer);
			result->setStatus(Result::Status::SUCCESS);
		}
	}

private:
	DeviceID m_deviceID;
};

/*
 * The handler supports the FakeCommand command. The task execution
 * is set to 60 ms and starts after the FakeHandler1.
 */
class FakeHandler2 : public CommandHandler {
public:
	FakeHandler2(const DeviceID &deviceID):
		m_deviceID(deviceID)
	{
	}

	bool accept(Command::Ptr cmd) override
	{
		if (cmd->is<FakeCommand>()) {
			return m_deviceID == cmd.cast<FakeCommand>()->deviceID();
		}

		return false;
	}

	void handle(Command::Ptr cmd, Answer::Ptr answer) override
	{
		if (cmd->is<FakeCommand>()) {
			Poco::Thread::sleep(60);

			Result::Ptr result = new Result(answer);
			result->setStatus(Result::Status::FAILED);
		}
	}

private:
	DeviceID m_deviceID;
};

class NonAcceptingCommandHandler : public CommandHandler {
public:
	NonAcceptingCommandHandler()
	{
	}

	bool accept(Command::Ptr) override
	{
		return false;
	}

	void handle(Command::Ptr, Answer::Ptr) override
	{
	}
};

class FakeCommandSender : public CommandSender, public CommandHandler {
public:
	FakeCommandSender(const DeviceID &deviceID):
		m_deviceID(deviceID)
	{
	}

	virtual ~FakeCommandSender()
	{
	}

	bool accept(Command::Ptr cmd) override
	{
		if (cmd->is<FakeCommand>()) {
			return m_deviceID == cmd.cast<FakeCommand>()->deviceID();
		}

		return false;
	}

	void handle(Command::Ptr cmd, Answer::Ptr answer) override
	{
		if (cmd->is<FakeCommand>()) {
			Poco::Thread::sleep(60);

			Result::Ptr result = new Result(answer);
			result->setStatus(Result::Status::FAILED);
		}
	}

private:
	DeviceID m_deviceID;
};

void CommandDispatcherTest::setUp()
{
	m_executor = new ParallelExecutor;
	m_executorThread.start(*m_executor);
}

void CommandDispatcherTest::tearDown()
{
	m_executor->stop();
	m_executorThread.join();
}

/*
 * The waiting for the response to the supported command in the
 * FakeHandler1 and the FakeHandler2.
 */
void CommandDispatcherTest::testSupportedCommand()
{
	DeviceID deviceID = DeviceID(0xfe01020304050607);
	AsyncCommandDispatcher dispatcher;
	dispatcher.setCommandsExecutor(m_executor);

	AnswerQueue queue;
	std::list<Answer::Ptr> answerList;

	FakeCommand::Ptr cmd = new FakeCommand(deviceID);
	Answer::Ptr answer = new Answer(queue);

	Poco::SharedPtr<CommandHandler> handlerTest1(new FakeHandler1(deviceID));
	Poco::SharedPtr<CommandHandler> handlerTest2(new FakeHandler2(deviceID));

	dispatcher.registerHandler(handlerTest1);
	dispatcher.registerHandler(handlerTest2);

	Poco::Timestamp now;
	dispatcher.dispatch(cmd, answer);

	CPPUNIT_ASSERT(!queue.wait(1, answerList));
	CPPUNIT_ASSERT(1 == queue.size());
	CPPUNIT_ASSERT(answerList.empty());

	// wait for result from FakeHandler1, execute in about 20 ms
	CPPUNIT_ASSERT(queue.wait(200*Poco::Timespan::MILLISECONDS, answerList));
	CPPUNIT_ASSERT(now.elapsed() >= 20000); // FakeHandler1 was executed
	CPPUNIT_ASSERT(now.elapsed() < 60000); // FakeHandler2 wasn't executed
	CPPUNIT_ASSERT(answerList.size() == 1);

	// wait for result from FakeHandler2, execute in about 50 ms
	CPPUNIT_ASSERT(queue.wait(200*Poco::Timespan::MILLISECONDS, answerList));
	CPPUNIT_ASSERT(now.elapsed() >= 60000); // FakeHandler2 was executed
	// it has been waiting for less than the timeout
	CPPUNIT_ASSERT(now.elapsed() < 200000);
	CPPUNIT_ASSERT(answerList.size() == 1);

	// check the set values from handlers
	CPPUNIT_ASSERT(answer->at(0)->status() == Result::Status::SUCCESS);
	CPPUNIT_ASSERT(answer->at(1)->status() == Result::Status::FAILED);

	// Answer will be served, answer contains 2 Results
	CPPUNIT_ASSERT(!answer->isEmpty());

	queue.remove(answer);
	Poco::ThreadPool::defaultPool().joinAll();
}

/*
 * The waiting for the response to the unsupported command
 * in the NonAcceptingCommandHandler.
 */
void CommandDispatcherTest::testUnsupportedCommand()
{
	AnswerQueue queue;
	AsyncCommandDispatcher dispatcher;
	dispatcher.setCommandsExecutor(m_executor);

	DeviceID deviceID = DeviceID(0xfe01020304050607);

	Poco::SharedPtr<CommandHandler> handlerTest1(new NonAcceptingCommandHandler());
	dispatcher.registerHandler(handlerTest1);

	FakeCommand::Ptr cmd = new FakeCommand(deviceID);
	std::list<Answer::Ptr> answerList;
	Answer::Ptr answer = new Answer(queue);

	dispatcher.dispatch(cmd, answer);

	// Answer was not served
	CPPUNIT_ASSERT(!answer->isPending());

	CPPUNIT_ASSERT(queue.wait(1, answerList));
	CPPUNIT_ASSERT(1 == answerList.size());

	// Answer will not be served, answer doesn't contain any Result
	CPPUNIT_ASSERT(answer->isEmpty());

	queue.remove(answer);
}

/*
 * Sending of command and verification, if handler that sent command
 * will not process sent command.
 */
void CommandDispatcherTest::testCommandSender()
{
	AnswerQueue queue;
	Poco::SharedPtr<AsyncCommandDispatcher> dispatcher(new AsyncCommandDispatcher());
	dispatcher->setCommandsExecutor(m_executor);

	DeviceID deviceID = DeviceID(0xfe01020304050607);

	Poco::SharedPtr<FakeCommandSender> commandSender(new FakeCommandSender(deviceID));
	dispatcher->registerHandler(commandSender);

	FakeCommand::Ptr cmd = new FakeCommand(deviceID);
	std::list<Answer::Ptr> answerList;
	Answer::Ptr answer = new Answer(queue);

	commandSender->setCommandDispatcher(dispatcher);
	commandSender->dispatch(cmd, answer);

	// Answer was not served
	CPPUNIT_ASSERT(!answer->isPending());

	CPPUNIT_ASSERT(queue.wait(100, answerList));
	CPPUNIT_ASSERT(1 == answerList.size());
	CPPUNIT_ASSERT(0 == answerList.front()->resultsCount());

	// Answer will not be served, answer doesn't contain any Result
	CPPUNIT_ASSERT(answer->isEmpty());

	queue.remove(answer);
}

}
