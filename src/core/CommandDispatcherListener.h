#pragma once

#include <Poco/SharedPtr.h>

#include "core/Command.h"

namespace BeeeOn {

/**
 * The base interface for listening events occured in CommandDispatcher.
 */
class CommandDispatcherListener {
public:
	typedef Poco::SharedPtr<CommandDispatcherListener> Ptr;

	CommandDispatcherListener();

	virtual ~CommandDispatcherListener();

	virtual void onDispatch(const Command::Ptr cmd) = 0;
};

}
