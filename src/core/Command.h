#ifndef BEEEON_COMMAND_H
#define BEEEON_COMMAND_H

#include <string>

#include <Poco/AutoPtr.h>
#include <Poco/RefCountedObject.h>
#include <Poco/SharedPtr.h>

#include "core/Answer.h"
#include "core/Result.h"
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

	Command();

	std::string name() const;

	/**
	 * Returns CommandHandler that initiates sending of this commands.
	 * If the sender does not implement the CommandHandler interface,
	 * it returns NULL.
	 */
	CommandHandler* sendingHandler() const;

	/**
	 * Converts Command to human readable format.
	 */
	virtual std::string toString() const;

	/**
	 * Derive result appropriate for the Command instance.
	 * The result is always created in the PENDING state.
	 *
	 * The default implementation returns an instance of class
	 * Result as it is suitable for most commands. Specific
	 * command would override this method to derive another
	 * Result (sub)class.
	 */
	virtual Result::Ptr deriveResult(Answer::Ptr answer) const;

protected:
	void setSendingHandler(CommandHandler *sender);

	virtual ~Command();

protected:
	CommandHandler *m_sendingHandler;
};

}

#endif
