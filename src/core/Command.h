#ifndef BEEEON_COMMAND_H
#define BEEEON_COMMAND_H

#include <string>

#include <Poco/AutoPtr.h>
#include <Poco/RefCountedObject.h>

#include "util/Castable.h"

namespace BeeeOn {

/*
 * Abstract class for Commands, which will be sent within the gates.
 *
 * All Command objects should have a protected destructor,
 * to forbid explicit use of delete.
 */
class Command : public Poco::RefCountedObject, public Castable {
public:
	typedef Poco::AutoPtr<Command> Ptr;

	Command(const std::string &commandName);

	std::string name() const;

protected:
	virtual ~Command();

protected:
	std::string m_commandName;
};

}

#endif
