#include <Poco/DateTimeFormatter.h>
#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "di/Injectable.h"
#include "gwmessage/GWSensorDataConfirm.h"
#include "gwmessage/GWSensorDataExport.h"
#include "server/GWSResender.h"

BEEEON_OBJECT_BEGIN(BeeeOn, GWSResender)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(GWSListener)
BEEEON_OBJECT_PROPERTY("connector", &GWSResender::setConnector)
BEEEON_OBJECT_PROPERTY("resendTimeout", &GWSResender::setResendTimeout)
BEEEON_OBJECT_END(BeeeOn, GWSResender)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

GWSResender::GWSResender():
	m_resendTimeout(10 * Timespan::SECONDS)
{
}

void GWSResender::setConnector(GWSConnector::Ptr connector)
{
	m_connector = connector;
}

void GWSResender::setResendTimeout(const Timespan &timeout)
{
	if (timeout <= 0)
		throw InvalidArgumentException("resendTimeout must be positive");

	m_resendTimeout = timeout;
}

void GWSResender::run()
{
	StopControl::Run run(m_stopControl);

	logger().information("starting GWS resender");

	while (run) {
		ScopedLockWithUnlock<FastMutex> guard(m_lock);

		if (m_waiting.empty()) {
			guard.unlock();
			m_event.wait();
			continue;
		}

		const Clock now;
		const auto current = resendOrGet(now);

		if (current != m_waiting.end()) {
			Timespan delay = current->first - now;
			if (delay < 1 * Timespan::MILLISECONDS)
				delay = 1 * Timespan::MILLISECONDS;

			if (logger().debug()) {
				logger().debug(
					"idle, resend of " + current->second->toBriefString()
					+ " after " + DateTimeFormatter::format(delay),
					__FILE__, __LINE__);
			}

			guard.unlock();
			m_event.tryWait(delay.totalMilliseconds());
		}
	}

	logger().information("GWS resender has stopped");
}

void GWSResender::stop()
{
	m_stopControl.requestStop();
	m_event.set();
}

GWSResender::WaitingList::const_iterator GWSResender::resendOrGet(const Clock &now)
{
	const auto it = m_waiting.begin();

	if (it == m_waiting.end() || it->first > now)
		return it;

	GWMessage::Ptr message = it->second;

	if (logger().debug()) {
		logger().debug(
			"resending message " + message->toBriefString(),
			__FILE__, __LINE__);
	}

	m_refs.erase(it->second->id());
	m_waiting.erase(it);

	try {
		m_connector->send(message);
	}
	BEEEON_CATCH_CHAIN(logger())

	return m_waiting.begin();
}

GWSResender::WaitingList &GWSResender::waiting()
{
	return m_waiting;
}

void GWSResender::onTrySend(const GWMessage::Ptr message)
{
	FastMutex::ScopedLock guard(m_lock);
	m_pending.emplace(message->id());
}

void GWSResender::onSent(const GWMessage::Ptr message)
{
	FastMutex::ScopedLock guard(m_lock);

	if (!resendable(message))
		return;

	if (m_pending.find(message->id()) == m_pending.end())
		return;

	auto it = m_refs.find(message->id());
	if (it != m_refs.end()) {
		GWResponse::Ptr orig = it->second->second.cast<GWResponse>();

		if (!orig.isNull()) {
			GWResponse::Ptr response = message.cast<GWResponse>();
			poco_assert_msg(!response.isNull(), "expected a response while resending");

			switch (orig->status()) {
			case GWResponse::Status::SUCCESS:
			case GWResponse::Status::FAILED:
				if (orig->status() == response->status())
					break;

				logger().warning(
					"attempt to override final response "
					+ orig->toBriefString()
					+ " by status "
					+ to_string(response->status()),
					__FILE__, __LINE__);
				return;
			default: // ACCEPTED can be overriden
				break;
			}
		}

		it->second->second = message;

		const Timespan remaining = m_resendTimeout - it->second->first.elapsed();

		if (logger().debug()) {
			logger().debug(
				"update message "
				+ message->toBriefString()
				+ " to be resent scheduled in "
				+ DateTimeFormatter::format(remaining),
				__FILE__, __LINE__);
		}

		return;
	}

	if (logger().debug()) {
		logger().debug(
			"schedule resend of " + message->toBriefString()
			+ " in " + DateTimeFormatter::format(m_resendTimeout),
			__FILE__, __LINE__);
	}

	Clock at;
	at += m_resendTimeout.totalMicroseconds();

	auto result = m_waiting.emplace(at, message);
	m_refs.emplace(message->id(), result);
	m_event.set();
}

void GWSResender::onResponse(const GWResponse::Ptr response)
{
	switch (response->status()) {
	case GWResponse::Status::SUCCESS:
	case GWResponse::Status::FAILED:
		break;
		
	default:
		return; // ignore
	}

	findAndDrop(response);
}

void GWSResender::onAck(const GWAck::Ptr ack)
{
	FastMutex::ScopedLock guard(m_lock);

	m_pending.erase(ack->id());

	auto it = m_refs.find(ack->id());
	if (it == m_refs.end())
		return;

	GWResponse::Ptr response = it->second->second.cast<GWResponse>();
	if (response.isNull()) {
		logger().warning(
			"attempt to ack message of type "
			+ response->type().toString()
			+ ", ignoring...",
			__FILE__, __LINE__);

		return;
	}

	if (response->status() != ack->status()) {
		if (logger().debug()) {
			logger().debug(
				"out-of-date ack of " + to_string(ack->status())
				+ " but current " + to_string(response->status())
				+ " for " + response->id().toString(),
				__FILE__, __LINE__);
		}

		return;
	}

	if (logger().debug()) {
		logger().debug(
			"response " + response->toBriefString() + " was acked",
			__FILE__, __LINE__);
	}

	m_waiting.erase(it->second);
	m_refs.erase(it);
}

void GWSResender::onOther(const GWMessage::Ptr message)
{
	if (message.cast<GWSensorDataConfirm>())
		findAndDrop(message);
}

bool GWSResender::resendable(const GWMessage::Ptr message) const
{
	if (!message.cast<GWRequest>().isNull())
		return true;

	if (message.cast<GWResponse>()) {
		GWResponse::Ptr response = message.cast<GWResponse>();
		return response->ackExpected();
	}

	if (!message.cast<GWSensorDataExport>().isNull())
		return true;

	return false;
}

void GWSResender::findAndDrop(const GWMessage::Ptr message)
{
	FastMutex::ScopedLock guard(m_lock);

	m_pending.erase(message->id());

	auto it = m_refs.find(message->id());
	if (it == m_refs.end())
		return;

	if (logger().debug()) {
		logger().debug(
			"message " + message->toBriefString() + " delivered",
			__FILE__, __LINE__);
	}

	m_waiting.erase(it->second);
	m_refs.erase(it);
}
