#pragma once

#include "core/CommandDispatcher.h"
#include "util/ParallelExecutor.h"

namespace BeeeOn {

/**
 * @brief AsyncCommandDispatcher implements dispatching of commands
 * via a ParallelExecutor instance.
 */
class AsyncCommandDispatcher : public CommandDispatcher {
public:
	void setCommandsExecutor(ParallelExecutor::Ptr executor);

protected:
	void dispatchImpl(Command::Ptr cmd, Answer::Ptr answer) override;

private:
	ParallelExecutor::Ptr m_commandsExecutor;
};

}
