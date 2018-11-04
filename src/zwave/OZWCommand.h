#pragma once

#include <Poco/SynchronizedObject.h>
#include <Poco/Timespan.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <openzwave/Driver.h>
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

#include "util/Loggable.h"

namespace BeeeOn {

class OZWNetwork;

/**
 * @brief OZWCommand handles OpenZWave command management.
 * It allows to request a command to be executed and takes
 * care of race conditions.
 */
class OZWCommand : public Poco::SynchronizedObject, Loggable {
public:
	enum Type {
		NONE,
		INCLUSION,
		REMOVE_NODE,
	};

	OZWCommand(OZWNetwork &ozw);

	/**
	 * @returns type of command that is currently in progress
	 */
	Type type() const;

	/**
	 * @returns true if a command has been requested and did not
	 * finished yet
	 */
	bool wasRequested() const;

	/**
	 * @returns true if a command has been requested and is already
	 * in progress (running)
	 */
	bool isRunning() const;

	/**
	 * @returns true if the current command is being cancelled (this
	 * can take some time)
	 */
	bool isCancelling() const;

	/**
	 * @brief Request the given type of command to execute in the context
	 * of the given Z-Wave home ID.
	 *
	 * @throw IllegalStateException when a command is already requested or running
	 * @throw IllegalStateException when the command has failed to start
	 */
	void request(Type type, uint32_t home);

	/**
	 * @brief Cancel the current command if it is of the given type. This allows
	 * to cancel e.g. INCLUSION without knowing whether INCLUSION is currently
	 * running. The cancel operation blocks at least for the given timeout.
	 * If the timeout exceeds, the cancelling operation is stopped.
	 *
	 * @returns true if the given type matches the currently running command
	 * and the cancel has succeededl
	 *
	 * @param type of operation to be cancelled
	 * @param timeout to block at maximum (must be at least 1 ms)
	 *
	 * @throw InvalidArgumentException if the given timeout is invalid
	 * @throw IllegalStateException when the command is already being cancelled
	 * @throw IllegalStateException when no command has been requested
	 */
	bool cancelIf(Type type, const Poco::Timespan &timeout);

protected:
	void onNormal();
	void onStarted();
	void onWaitUser();
	void onInProgress();
	void onSleeping();
	void onCancelled();
	void onError(OpenZWave::Driver::ControllerError error);
	void onFailed();
	void onSuccess();
	void onNodeOK();
	void onNodeFailed();

	static void handle(
		OpenZWave::Driver::ControllerState state,
		OpenZWave::Driver::ControllerError error,
		void *context);

private:
	std::string typeName() const;
	std::string typeName(Type type) const;
	void stopCancelling();
	void running();
	void terminated();

private:
	Type m_type;
	bool m_requested;
	bool m_running;
	bool m_cancelling;
	uint32_t m_home;
	Poco::Event m_event;
	OZWNetwork &m_ozw;
};

}
