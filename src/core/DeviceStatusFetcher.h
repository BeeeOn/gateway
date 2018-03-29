#pragma once

#include <map>
#include <set>

#include <Poco/Clock.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>

#include "core/CommandSender.h"
#include "core/DeviceStatusHandler.h"
#include "loop/StoppableRunnable.h"
#include "loop/StopControl.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief DeviceStatusFetcher is responsible for fetching pairing state of
 * devices for the registered status handlers. The fetching is performed
 * asynchronously and independently resulting in calling to the method
 * DeviceStatusHandler::handleRemoteStatus() on the appropriate handlers.
 */
class DeviceStatusFetcher :
	public CommandSender,
	public StoppableRunnable,
	Loggable {
public:
	typedef Poco::SharedPtr<DeviceStatusFetcher> Ptr;

	DeviceStatusFetcher();

	/**
	 * @brief Set duration how long to sleep while there is nothing to do.
	 */
	void setIdleDuration(const Poco::Timespan &duration);

	/**
	 * @brief Set timeout for AnswerQueue::wait() call.
	 */
	void setWaitTimeout(const Poco::Timespan &timeout);

	/**
	 * @brief Set timeout to wait until a request is re-issued after its
	 * unsuccessful finish (Answer was not fully successful).
	 */
	void setRepeatTimeout(const Poco::Timespan &timeout);

	/**
	 * @brief Register the given device status handler. The DeviceStatusFetcher
	 * would request a remote pairing registry (server) for the paired devices
	 * for that handler.
	 */
	void registerHandler(DeviceStatusHandler::Ptr handler);

	/**
	 * @brief Unregister all registered device status handlers.
	 */
	void clearHandlers();

	void run() override;
	void stop() override;

protected:
	/**
	 * @brief To simplify Answer management, include the prefix
	 * of the connected ServerDeviceListCommand inside the answer.
	 */
	class PrefixAnswer : public Answer {
	public:
		typedef Poco::AutoPtr<PrefixAnswer> Ptr;

		PrefixAnswer(
			AnswerQueue &queue,
			const DevicePrefix &prefix);

		DevicePrefix prefix() const;

	private:
		const DevicePrefix m_prefix;
	};

	class PrefixStatus {
	public:
		PrefixStatus();

		/**
		 * @brief Update the status of an associated prefix that
		 * the request for the associated prefix has been initiated.
		 */
		void startRequest();

		/**
		 * @brief Update the status of an associated prefix that an
		 * appropriate response has been delivered. The parameter
		 * denotes whether the response has been fully successful or
		 * not. This influences whether the request would be repeated.
		 */
		void deliverResponse(bool successful);

		/**
		 * @returns true if the status for the associated prefix needs
		 * to be requested (not requested yet, or not fully successful).
		 */
		bool needsRequest() const;

		/**
		 * @brief When the response was not fully successful, the request
		 * should be repeated. However, we do not want to DoS the remote
		 * server. Thus, the repeated requested would be performed after
		 * the given repeat timeout. The method returns true only, if the
		 * last response was not fully successful and the timeout exceeded
		 * sice the last request.
		 *
		 * @returns true whether the request of the status should be repeated
		 */
		bool shouldRepeat(const Poco::Timespan &repeatTimeout) const;

	private:
		Poco::Clock m_lastRequested;
		bool m_successful;
	};

	/**
	 * @brief Denote certain situations of the status fetching.
	 */
	enum FetchStatus {
		NOTHING,
		WOULD_REPEAT,
		ACTIVE
	};

protected:
	/**
	 * @brief Determine status handlers for which no fully successful
	 * request was made and dispatch ServerDeviceListCommand for them.
	 *
	 * @returns whether there something to do
	 */
	FetchStatus fetchUndone();

	/**
	 * @brief Check status of the given answer and if it is not pending
	 * and castable to PrefixAnswer, it performs cast and returns it.
	 *
	 * @returns instance of PrefixAnswer if it should be processed or null
	 */
	PrefixAnswer::Ptr handleDirtyAnswer(Answer::Ptr answer);

	/**
	 * @return status handler matching the given answer (by prefix)
	 */
	std::set<DeviceStatusHandler::Ptr> matchHandlers(
			const PrefixAnswer::Ptr answer) const;

	/**
	 * @brief Process results of the given answer and the matching status handler.
	 * When any of the results is unsuccessful, all the answer is considered
	 * unsuccessful and it would be marked for repeated fetch.
	 *
	 * If there is at least one successful result, the method
	 * DeviceStatusHandler::handleRemoteStatus() is called once on
	 * the given handler as a result of the answer processing.
	 */
	void processAnswer(
		PrefixAnswer::Ptr answer,
		std::set<DeviceStatusHandler::Ptr> handler);

	/**
	 * @brief Process paired devices as given in a single Answer result.
	 * Only devices matching the given prefix would be used. Other devices
	 * are considered as a bug somewhere.
	 */
	void collectPaired(
		std::set<DeviceID> &paired,
		const std::vector<DeviceID> &received,
		const DevicePrefix &prefix) const;

private:
	StopControl m_stopControl;
	Poco::Timespan m_idleDuration;
	Poco::Timespan m_waitTimeout;
	Poco::Timespan m_repeatTimeout;
	std::map<DevicePrefix, std::set<DeviceStatusHandler::Ptr>> m_handlers;
	std::map<DevicePrefix, PrefixStatus> m_status;
};

}
