#ifndef BEEEON_TESTING_CENTER_H
#define BEEEON_TESTING_CENTER_H

#include <functional>
#include <map>
#include <vector>

#include <Poco/AtomicCounter.h>
#include <Poco/SharedPtr.h>

#include "core/AnswerQueue.h"
#include "core/CommandDispatcher.h"
#include "io/Console.h"
#include "loop/StoppableRunnable.h"
#include "util/Loggable.h"

namespace BeeeOn {

class TestingCenter : public StoppableRunnable, protected Loggable {
public:
	struct ActionContext {
		ConsoleSession &console;
		AnswerQueue &queue;
		Poco::SharedPtr<CommandDispatcher> dispatcher;
		const std::vector<std::string> args;
	};

	/**
	 * Action to be executed when processing console input.
	 */
	typedef std::function<void(ActionContext &context)> Action;

	struct ActionRecord {
		std::string description;
		Action action;
	};

	TestingCenter();

	void run() override;
	void stop() override;

	void setCommandDispatcher(Poco::SharedPtr<CommandDispatcher> dispatcher);
	Poco::SharedPtr<CommandDispatcher> commandDispatcher() const;
	void setConsole(Poco::SharedPtr<Console> console);
	Poco::SharedPtr<Console> console() const;

protected:
	void registerAction(
			const std::string &name,
			const Action action,
			const std::string &description);
	void printHelp(ConsoleSession &session);
	void processLine(ConsoleSession &session, const std::string &line);

private:
	AnswerQueue m_queue;
	Poco::SharedPtr<CommandDispatcher> m_dispatcher;
	Poco::SharedPtr<Console> m_console;
	Poco::AtomicCounter m_stop;
	std::map<std::string, ActionRecord> m_action;
};

}

#endif
