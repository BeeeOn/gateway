#pragma once

#include <map>
#include <set>

#include <Poco/Clock.h>
#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/Timespan.h>

#include "loop/StoppableRunnable.h"
#include "loop/StopControl.h"
#include "server/GWSConnector.h"
#include "server/GWSListener.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief Sending of messages via a GWSConnector might not be reliable.
 * Messages that have been sent can never reach the remote server.
 * The GWSResender maintains the sent messages (where a reply is expected).
 * When no response or ack is received on time, such message is sent
 * again.
 *
 * If a message of an existing ID is to be resent, it replaces the previous
 * message of the same ID scheduled for resent. Thus, only the most recent
 * message of the same ID is always scheduled.
 */
class GWSResender :
	public StoppableRunnable,
	public GWSListener,
	Loggable {
public:
	GWSResender();

	void setConnector(GWSConnector::Ptr connector);

	/**
	 * @brief Configure timeout used to delay each resend.
	 */
	void setResendTimeout(const Poco::Timespan &timeout);

	/**
	 * @brief Implement scheduler of the waiting messages.
	 */
	void run() override;
	void stop() override;

	void onTrySend(const GWMessage::Ptr message) override;

	/**
	 * @brief Put the given message into the waiting list
	 * if it is re-sendable. After resendTimeout, such message
	 * is send again unless an appropriate response/ack has
	 * been delivered.
	 *
	 * @see GWSResender::resendable()
	 */ 
	void onSent(const GWMessage::Ptr message) override;

	void onResponse(const GWResponse::Ptr response) override;
	void onAck(const GWAck::Ptr ack) override;

	/**
	 * @brief Process GWSensorDataConfirm messages.
	 */ 
	void onOther(const GWMessage::Ptr message) override;

protected:
	typedef std::multimap<Poco::Clock, GWMessage::Ptr> WaitingList;

	/**
	 * @brief Resend the oldest message waiting for resend
	 * if its timeout has expired.
	 * @returns the oldest message that is to be resent
	 */
	WaitingList::const_iterator resendOrGet(const Poco::Clock &now);

	/**
	 * @returns the container of waiting messages
	 */
	WaitingList &waiting();

	/**
	 * @brief Certain messages should be resended when there
	 * is no response/ack during the resendTimeout period.
	 * This applies to requests, responses with ack and
	 * sensor-data-export.
	 *
	 * @returns true if the given message is re-sendable.
	 */ 
	bool resendable(const GWMessage::Ptr message) const;

	/**
	 * @brief Find the given message in the waiting list
	 * and remove it. Such message is considered as delivered
	 * or gracefully failed.
	 */ 
	void findAndDrop(const GWMessage::Ptr message);

private:
	GWSConnector::Ptr m_connector;
	Poco::Timespan m_resendTimeout;
	WaitingList m_waiting;
	std::map<GlobalID, WaitingList::iterator> m_refs;
	std::set<GlobalID> m_pending;
	StopControl m_stopControl;
	Poco::Event m_event;
	Poco::FastMutex m_lock;
};

}
