#include <cppunit/extensions/HelperMacros.h>

#include "core/Command.h"

namespace BeeeOn {

class CommandsTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(CommandsTest);
	CPPUNIT_TEST(testTypeAndCast);
	CPPUNIT_TEST_SUITE_END();

public:
	void testTypeAndCast();
};

CPPUNIT_TEST_SUITE_REGISTRATION(CommandsTest);

class FakeCommand : public Command {
public:
	FakeCommand(const std::string &name):
		Command(name)
	{
	}
};

/*
 * Checking whether the method is correctly detects the type of command.
 */
void CommandsTest::testTypeAndCast()
{
	FakeCommand fakeCommand("FakeCommand");

	CPPUNIT_ASSERT(!fakeCommand.is<Command>());
	CPPUNIT_ASSERT(fakeCommand.is<FakeCommand>());
	CPPUNIT_ASSERT(!fakeCommand.is<Command>());

	Command *cmdPtr = &fakeCommand;
	CPPUNIT_ASSERT(cmdPtr->is<FakeCommand>());
}

}
