#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "commands/ServerDeviceListCommand.h"
#include "commands/ServerDeviceListResult.h"
#include "core/DeviceStatusFetcher.h"
#include "core/CommandDispatcher.h"
#include "di/Injectable.h"

BEEEON_OBJECT_BEGIN(BeeeOn, DeviceStatusFetcher)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_PROPERTY("idleDuration", &DeviceStatusFetcher::setIdleDuration)
BEEEON_OBJECT_PROPERTY("waitTimeout", &DeviceStatusFetcher::setWaitTimeout)
BEEEON_OBJECT_PROPERTY("repeatTimeout", &DeviceStatusFetcher::setRepeatTimeout)
BEEEON_OBJECT_PROPERTY("commandDispatcher", &DeviceStatusFetcher::setCommandDispatcher)
BEEEON_OBJECT_PROPERTY("handlers", &DeviceStatusFetcher::registerHandler)
BEEEON_OBJECT_HOOK("cleanup", &DeviceStatusFetcher::clearHandlers)
BEEEON_OBJECT_END(BeeeOn, DeviceStatusFetcher)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

DeviceStatusFetcher::PrefixAnswer::PrefixAnswer(
		AnswerQueue &queue,
		const DevicePrefix &prefix):
	Answer(queue),
	m_prefix(prefix)
{
}

DevicePrefix DeviceStatusFetcher::PrefixAnswer::prefix() const
{
	return m_prefix;
}

DeviceStatusFetcher::PrefixStatus::PrefixStatus():
	m_lastRequested(0),
	m_started(false),
	m_successful(false)
{
}

void DeviceStatusFetcher::PrefixStatus::startRequest()
{
	poco_assert(!m_successful);
	m_lastRequested.update();
	m_started = true;
}

void DeviceStatusFetcher::PrefixStatus::deliverResponse(
		bool successful)
{
	poco_assert(!m_successful);
	m_successful = successful;
}

bool DeviceStatusFetcher::PrefixStatus::needsRequest() const
{
	return !m_started;
}

bool DeviceStatusFetcher::PrefixStatus::shouldRepeat(
		const Timespan &repeatTimeout) const
{
	if (m_successful)
		return false;

	if (!m_lastRequested.isElapsed(repeatTimeout.totalMicroseconds()))
		return false;

	return true;
}

DeviceStatusFetcher::DeviceStatusFetcher():
	m_idleDuration(30 * Timespan::MINUTES),
	m_waitTimeout(1 * Timespan::SECONDS),
	m_repeatTimeout(5 * Timespan::MINUTES)
{
}

void DeviceStatusFetcher::setIdleDuration(const Timespan &duration)
{
	if (duration < 1 * Timespan::SECONDS)
		throw InvalidArgumentException("idleDuration must be at least 1 s");

	m_idleDuration = duration;
}

void DeviceStatusFetcher::setWaitTimeout(const Timespan &timeout)
{
	if (timeout < 1 * Timespan::MILLISECONDS)
		throw InvalidArgumentException("waitTimeout must be at least 1 ms");

	m_waitTimeout = timeout;
}

void DeviceStatusFetcher::setRepeatTimeout(const Timespan &timeout)
{
	if (timeout < 1 * Timespan::MILLISECONDS)
		throw InvalidArgumentException("repeatTimeout must be at least 1 ms");

	m_repeatTimeout = timeout;
}

void DeviceStatusFetcher::registerHandler(DeviceStatusHandler::Ptr handler)
{
	auto result = m_handlers.emplace(handler->prefix(), set<DeviceStatusHandler::Ptr>{handler});
	if (!result.second) {
		set<DeviceStatusHandler::Ptr> &handlers = result.first->second;
		handlers.emplace(handler);
	}
}

void DeviceStatusFetcher::clearHandlers()
{
	m_handlers.clear();
}

DeviceStatusFetcher::FetchStatus DeviceStatusFetcher::fetchUndone()
{
	if (m_status.empty()) {
		for (const auto &pair : m_handlers)
			m_status.emplace(pair.first, PrefixStatus());
	}

	bool wouldRepeat = false;
	bool started = false;

	for (auto &pair : m_status) {
		const auto &prefix = pair.first;
		auto &status = pair.second;

		if (!status.needsRequest()) {
			if (status.shouldRepeat(m_repeatTimeout))
				wouldRepeat = true;

			continue;
		}

		if (logger().debug()) {
			logger().debug("fetching paired devices for " + prefix.toString(),
					__FILE__, __LINE__);
		}

		ServerDeviceListCommand::Ptr cmd = new ServerDeviceListCommand(prefix);
		dispatch(cmd, new PrefixAnswer(answerQueue(), prefix));

		status.startRequest();
		started = true;
	}

	if (started)
		return ACTIVE;
	if (wouldRepeat)
		return WOULD_REPEAT;

	return NOTHING;
}

DeviceStatusFetcher::PrefixAnswer::Ptr DeviceStatusFetcher::handleDirtyAnswer(
		Answer::Ptr answer)
{
	if (answer->isPending()) {
		if (logger().debug()) {
			logger().debug("answer is pending",
				__FILE__, __LINE__);
		}

		return NULL;
	}

	answerQueue().remove(answer);

	if (answer->handlersCount() == 0) {
		logger().warning("answer has no handlers",
				__FILE__, __LINE__);
		return NULL;
	}

	PrefixAnswer::Ptr prefixAnswer = answer.cast<PrefixAnswer>();
	if (prefixAnswer.isNull()) {
		logger().warning("received answer is not PrefixAnswer",
				__FILE__, __LINE__);
		return NULL;
	}

	return prefixAnswer;
}

set<DeviceStatusHandler::Ptr> DeviceStatusFetcher::matchHandlers(
		const PrefixAnswer::Ptr answer) const
{
	auto it = m_handlers.find(answer->prefix());
	if (it == m_handlers.end())
		return {};

	return it->second;
}

void DeviceStatusFetcher::run()
{
	logger().information("starting device fetcher...");

	StopControl::Run run(m_stopControl);

	while (run) {
		switch (fetchUndone()) {
		case NOTHING:
			if (answerQueue().size() == 0) {
				if (logger().debug()) {
					logger().debug(
						"nothing to do, sleeping...",
						__FILE__, __LINE__);
				}

				run.waitStoppable(m_idleDuration);
				continue;
			}
			break;

		case WOULD_REPEAT:
			if (answerQueue().size() == 0) {
				if (logger().debug()) {
					logger().debug(
						"would repeat some, sleeping now...",
						__FILE__, __LINE__);
				}

				run.waitStoppable(m_repeatTimeout);
				continue;
			}

			break;

		case ACTIVE:
			if (logger().debug()) {
				logger().debug("some request is still active", __FILE__, __LINE__);
			}

		default:
			break;
		}

		list<Answer::Ptr> dirty;
		answerQueue().wait(m_waitTimeout, dirty);

		if (dirty.empty())
			continue;

		if (logger().debug()) {
			logger().debug(
				"processing " + to_string(dirty.size()) + " answers",
				__FILE__, __LINE__);
		}

		for (Answer::Ptr answer : dirty) {
			Answer::ScopedLock guard(*answer);

			auto prefixAnswer = handleDirtyAnswer(answer);
			const auto handlers = matchHandlers(prefixAnswer);

			if (handlers.empty()) {
				logger().warning("no handlers for prefix "
						+ prefixAnswer->prefix().toString(),
						__FILE__, __LINE__);
				continue;
			}

			processAnswer(prefixAnswer, handlers);
		}
	}
}

void DeviceStatusFetcher::processAnswer(
		PrefixAnswer::Ptr answer,
		set<DeviceStatusHandler::Ptr> handlers)
{
	set<DeviceID> paired;
	bool failed = false;
	bool success = false;

	size_t i = 0;

	for (const auto result : *answer) {
		i += 1;

		if (result->status() != Result::Status::SUCCESS) {
			logger().warning(
				"result " + to_string(i) + "/" + to_string(answer->resultsCount())
				+ " has failed",
				__FILE__, __LINE__);

			failed = true;
			continue;
		}
		else {
			success = true;
		}

		ServerDeviceListResult::Ptr data = result.cast<ServerDeviceListResult>();
		if (data.isNull()) {
			logger().warning("result is not ServerDeviceListResult",
					__FILE__, __LINE__);
			continue;
		}

		collectPaired(paired, data->deviceList(), answer->prefix());
	}

	auto status = m_status.find(answer->prefix());
	poco_assert(status != m_status.end()); // it MUST be there

	status->second.deliverResponse(!failed);

	if (success && failed) {
		if (logger().debug()) {
			logger().debug(
				"answer was partially successful "
				"and thus the request would be repeated",
				__FILE__, __LINE__);
		}
	}

	if (!success)
		return;

	for (auto handler : handlers)
		handler->handleRemoteStatus(answer->prefix(), paired, {});
}

void DeviceStatusFetcher::collectPaired(
		std::set<DeviceID> &paired,
		const std::vector<DeviceID> &received,
		const DevicePrefix &prefix) const
{
	for (const auto &id : received) {
		if (id.prefix() != prefix) {
			logger().warning(
				"ID " + id.toString()
				+ " has not prefix "
				+ prefix.toString(),
				__FILE__, __LINE__);

			continue;
		}

		if (logger().trace()) {
			logger().trace("received ID " + id.toString(),
					__FILE__, __LINE__);
		}

		paired.emplace(id);
	}
}

void DeviceStatusFetcher::stop()
{
	m_stopControl.requestStop();
	answerQueue().dispose();
}
