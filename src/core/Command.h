#ifndef BEEEON_COMMAND_H
#define BEEEON_COMMAND_H

#include <string>

#include <Poco/AutoPtr.h>
#include <Poco/RefCountedObject.h>
#include <Poco/SharedPtr.h>

#include "util/Castable.h"

namespace BeeeOn {

class CommandDispatcher;
class CommandHandler;
class CommandSender;

/*
 * Abstract class for Commands, which will be sent within the gates.
 *
 * All Command objects should have a protected destructor,
 * to forbid explicit use of delete.
 */
class Command : public Poco::RefCountedObject, public Castable {
	// This way, we are hiding setSendingHandler() from the outer world.
	friend CommandDispatcher;
	friend CommandSender;
public:
	typedef Poco::AutoPtr<Command> Ptr;

	Command(const std::string &commandName);

	std::string name() const;

	/**
	 * Returns CommandHandler that initiates sending of this commands.
	 * If the sender does not implement the CommandHandler interface,
	 * it returns NULL.
	 */
	CommandHandler* sendingHandler() const;

protected:
	void setSendingHandler(CommandHandler *sender);

	virtual ~Command();

protected:
	std::string m_commandName;
	CommandHandler *m_sendingHandler;
};

}

#endif
