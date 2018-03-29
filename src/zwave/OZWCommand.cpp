#include <Poco/Exception.h>
#include <Poco/Logger.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <openzwave/Manager.h>
#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

#include "zwave/OZWCommand.h"
#include "zwave/OZWNetwork.h"

using namespace std;
using namespace OpenZWave;
using namespace Poco;
using namespace BeeeOn;

static bool ozwBeginControllerCommand(
	uint32_t home,
	Driver::ControllerCommand cmd,
	Driver::pfnControllerCallback_t callback,
	void *context,
	bool highPower,
	uint8_t node,
	uint8_t arg)
{
	/*
	 * There is currently no reliable way to handle command progress
	 * other then calling the OZW method named BeginControllerCommand.
	 * It accepts a callback function with context. However, this OZW
	 * method has been deprecated with no suitable replacement.
	 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	return Manager::Get()->BeginControllerCommand(
		home, cmd, callback, context, highPower, node, arg);
#pragma GCC diagnostic pop
}

void OZWCommand::handle(
		Driver::ControllerState state,
		Driver::ControllerError error,
		void *context)
{
	OZWCommand *cmd = reinterpret_cast<OZWCommand *>(context);

	try {
		switch (state) {
		case Driver::ControllerState_Starting:
			cmd->onStarted();
			break;
		case Driver::ControllerState_Waiting:
			cmd->onWaitUser();
			break;
		case Driver::ControllerState_InProgress:
			cmd->onInProgress();
			break;
		case Driver::ControllerState_Sleeping:
			cmd->onSleeping();
			break;
		case Driver::ControllerState_Cancel:
			cmd->onCancelled();
			break;
		case Driver::ControllerState_Error:
			cmd->onError(error);
			break;
		case Driver::ControllerState_Failed:
			cmd->onFailed();
			break;
		case Driver::ControllerState_Completed:
			cmd->onSuccess();
			break;
		case Driver::ControllerState_NodeOK:
			cmd->onNodeOK();
			break;
		case Driver::ControllerState_NodeFailed:
			cmd->onNodeFailed();
			break;
		case Driver::ControllerState_Normal:
			cmd->onNormal();
			break;
		}
	}
	BEEEON_CATCH_CHAIN(Loggable::forClass(typeid(OZWCommand)))
}

OZWCommand::OZWCommand(OZWNetwork &ozw):
	m_type(NONE),
	m_requested(false),
	m_running(false),
	m_cancelling(false),
	m_ozw(ozw)
{
}

void OZWCommand::request(Type type, uint32_t home)
{
	ScopedLock guard(*this);

	if (m_requested && !m_running) {
		throw IllegalStateException(
			"command " + typeName()
			+ " is currently being requested");
	}

	if (m_running) {
		throw IllegalStateException(
			"command " + typeName()
			+ " is currently running");
	}

	Driver::ControllerCommand cmd;
	bool highPower = false;

	switch (type) {
	case INCLUSION:
		cmd = Driver::ControllerCommand_AddDevice;
		highPower = true;
		break;
	case REMOVE_NODE:
		cmd = Driver::ControllerCommand_RemoveDevice;
		highPower = true;
		break;
	default:
		cmd = Driver::ControllerCommand_None;
		break;
	}

	m_type = type;
	m_requested = true;
	m_running = false;
	m_cancelling = false;
	m_home = home;

	bool ret = ozwBeginControllerCommand(
		home, cmd, &OZWCommand::handle, this, highPower, 0, 0);

	if (!ret) {
		throw IllegalStateException(
				"request of command "
				+ to_string(type)
				+ " has failed");
	}
}

bool OZWCommand::cancelIf(Type type, const Timespan &timeout)
{
	ScopedLockWithUnlock<OZWCommand> guard(*this);

	if (timeout < 1 * Timespan::MILLISECONDS)
		throw InvalidArgumentException("cancel timeout must be at least 1 ms");

	if (m_type == NONE)
		return false;

	if (logger().debug()) {
		logger().debug(
			"attempt to cancel command " + typeName(type),
			__FILE__, __LINE__);
	}

	if (m_type != type)
		return false;

	if (m_cancelling)
		throw IllegalStateException("cancelling already in progress");

	if (m_running) {
		Manager::Get()->CancelControllerCommand(m_home);
		return true;
	}

	if (!m_requested) {
		throw IllegalStateException(
			"cannot cancel command "
			+ typeName(type)
			+ " when it is not requested");
	}

	// requested but not running or cancelling yet

	const Clock started;

	m_cancelling = true;

	guard.unlock();

	while (!started.isElapsed(timeout.totalMicroseconds())) {
		Timespan remaining = timeout - started.elapsed();
		if (remaining.totalMilliseconds() < 1)
			remaining = 1 * Timespan::MILLISECONDS;

		m_event.tryWait(remaining.totalMilliseconds());

		OZWCommand::ScopedLock tmpGuard(*this);

		if (!isCancelling())
			return true;

		if (isRunning()) {
			Manager::Get()->CancelControllerCommand(m_home);
			stopCancelling();
			return true;
		}
	}

	stopCancelling();
	throw TimeoutException("cancelling of " + typeName(type));
}

void OZWCommand::stopCancelling()
{
	ScopedLock guard(*this);
	m_cancelling = false;
}

OZWCommand::Type OZWCommand::type() const
{
	ScopedLock guard(const_cast<OZWCommand &>(*this));
	return m_type;
}

string OZWCommand::typeName(Type type) const
{
	switch (type) {
	case NONE:
		return "none";
	case INCLUSION:
		return "inclusion";
	case REMOVE_NODE:
		return "remove-node";
	default:
		return "(unknown)";
	}
}

string OZWCommand::typeName() const
{
	return typeName(type());
}

bool OZWCommand::wasRequested() const
{
	ScopedLock guard(const_cast<OZWCommand &>(*this));
	return m_requested;
}

bool OZWCommand::isRunning() const
{
	ScopedLock guard(const_cast<OZWCommand &>(*this));
	return m_running;
}

bool OZWCommand::isCancelling() const
{
	ScopedLock guard(const_cast<OZWCommand &>(*this));
	return m_cancelling;
}

void OZWCommand::onNormal()
{
	ScopedLock guard(*this);

	if (!m_requested && !m_running)
		return;

	logger().information(
		"command " + typeName()
		+ " was done, nothing in progress",
		__FILE__, __LINE__);

	terminated();
}

void OZWCommand::onStarted()
{
	ScopedLock guard(*this);

	if (logger().debug()) {
		logger().debug(
			"command " + typeName() + " has started",
			__FILE__, __LINE__);
	}

	running();
}

void OZWCommand::onWaitUser()
{
	ScopedLock guard(*this);

	logger().information(
		"command " + typeName() + " is waiting for user",
		__FILE__, __LINE__);

	running();
}

void OZWCommand::onInProgress()
{
	ScopedLock guard(*this);

	if (logger().debug()) {
		logger().debug(
			"command " + typeName() + " is communicating",
			__FILE__, __LINE__);
	}

	running();
}

void OZWCommand::onSleeping()
{
	ScopedLock guard(*this);

	if (logger().debug()) {
		logger().debug(
			"command " + typeName()
			+ " is sleeping",
			__FILE__, __LINE__);
	}

	terminated();
}

void OZWCommand::onCancelled()
{
	ScopedLock guard(*this);

	logger().information(
		"command " + typeName()
		+ " was cancelled",
		__FILE__, __LINE__);

	terminated();
}

void OZWCommand::onError(Driver::ControllerError error)
{
	ScopedLock guard(*this);

	logger().error(
		"command " + typeName()
		+ " was aborted: " + to_string(error),
		__FILE__, __LINE__);

	terminated();
}

void OZWCommand::onFailed()
{
	ScopedLock guard(*this);

	logger().error(
		"command " + typeName() + " has failed",
		__FILE__, __LINE__);

	terminated();
}

void OZWCommand::onSuccess()
{
	ScopedLock guard(*this);

	logger().information(
		"command " + typeName() + " has succeeded",
		__FILE__, __LINE__);

	terminated();
}

void OZWCommand::onNodeOK()
{
	ScopedLock guard(*this);
	terminated();
}

void OZWCommand::onNodeFailed()
{
	ScopedLock guard(*this);
	terminated();
}

void OZWCommand::running()
{
	if (!m_running) {
		switch (m_type) {
		case INCLUSION:
			break;
		case REMOVE_NODE:
			break;
		default:
			break;
		}
	}

	m_running = true;
	m_event.set();
}

void OZWCommand::terminated()
{
	const Type type = m_type;
	const bool wasRunning = m_running;

	m_type = NONE;
	m_running = false;
	m_requested = false;
	m_event.set();

	if (wasRunning) {
		switch (type) {
		case INCLUSION:
			break;
		case REMOVE_NODE:
			break;
		default:
			break;
		}
	}
}

