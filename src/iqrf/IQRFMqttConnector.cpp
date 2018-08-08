#include <Poco/Logger.h>

#include "di/Injectable.h"
#include "iqrf/IQRFMqttConnector.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

BEEEON_OBJECT_BEGIN(BeeeOn, IQRFMqttConnector)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_PROPERTY("mqttClient", &IQRFMqttConnector::setMqttClient)
BEEEON_OBJECT_PROPERTY("publishTopic", &IQRFMqttConnector::setPublishTopic)
BEEEON_OBJECT_PROPERTY("dataTimeout", &IQRFMqttConnector::setDataTimeout)
BEEEON_OBJECT_PROPERTY("receiveTimeout", &IQRFMqttConnector::setReceiveTimeout)
BEEEON_OBJECT_HOOK("done", &IQRFMqttConnector::checkPublishTopic)
BEEEON_OBJECT_END(BeeeOn, IQRFMqttConnector)

IQRFMqttConnector::IQRFMqttConnector():
	m_messageTimeout(10 * Timespan::SECONDS),
	m_receiveTimeout(10 * Timespan::SECONDS)
{
}

void IQRFMqttConnector::setMqttClient(MqttClient::Ptr mqttClient)
{
	m_mqttClient = mqttClient;
}

void IQRFMqttConnector::setPublishTopic(const string &topic)
{
	m_publishTopic = topic;
}

void IQRFMqttConnector::setDataTimeout(const Timespan &timeout)
{
	if (timeout < 1 * Timespan::MILLISECONDS)
		throw InvalidArgumentException("dataTimeout must be at least 1 ms");

	m_messageTimeout = timeout;
}

void IQRFMqttConnector::setReceiveTimeout(const Timespan &timeout)
{
	if (timeout < 1 * Timespan::MILLISECONDS)
		throw InvalidArgumentException("receiveTimeout must be at least 1 ms");

	m_receiveTimeout = timeout;
}

void IQRFMqttConnector::checkPublishTopic()
{
	if (m_publishTopic.empty())
		InvalidArgumentException("mqtt publish topic is empty");
}

void IQRFMqttConnector::run()
{
	StopControl::Run run(m_stopControl);

	while (run) {
		MqttMessage msg;

		try {
			msg = m_mqttClient->receive(m_receiveTimeout);
		}
		catch (const TimeoutException &) {
			continue;
		}
		BEEEON_CATCH_CHAIN(logger())

		removeExpiredMessages(m_messageTimeout);

		if (msg.message().empty())
			continue;

		IQRFJsonResponse::Ptr response;
		try {
			auto iqrfJsonMsg = IQRFJsonResponse::parse(msg.message());
			response = iqrfJsonMsg.cast<IQRFJsonResponse>();
		}
		BEEEON_CATCH_CHAIN_ACTION(logger(), continue)

		FastMutex::ScopedLock guard(m_dataLock);
		const ReceivedData data = {Clock(), response};

		auto it = m_data.emplace(
			GlobalID::parse(response->messageID()),
			data);

		if (!it.second) {
			logger().warning(
				"duplicated message id " + response->messageID(),
				__FILE__, __LINE__);

			continue;
		}

		m_waitCondition.broadcast();
	}
}

void IQRFMqttConnector::stop()
{
	m_stopControl.requestStop();
	m_waitCondition.broadcast();
}

void IQRFMqttConnector::send(const string &msg)
{
	m_mqttClient->publish({m_publishTopic, msg});
}

IQRFJsonResponse::Ptr IQRFMqttConnector::receive(
		const GlobalID &id,
		const Timespan &timeout)
{
	const Clock now;

	while (!m_stopControl.shouldStop()) {
		ScopedLockWithUnlock<FastMutex> guard(m_dataLock);

		auto it = m_data.find(id);
		if (it != m_data.end()) {
			const auto message = it->second.message;
			m_data.erase(it);

			return message;
		}
		guard.unlock();

		if (timeout < 0) {
			m_waitCondition.wait();
		}
		else {
			Timespan waitTime = timeout.totalMicroseconds() - now.elapsed();

			if (waitTime <= 0)
				throw TimeoutException("receive timeout expired");

			if (waitTime.totalMilliseconds() < 1)
				waitTime = 1 * Timespan::MILLISECONDS;

			m_waitCondition.wait(waitTime);
		}
	}

	return {};
}

void IQRFMqttConnector::removeExpiredMessages(const Timespan &timeout)
{
	FastMutex::ScopedLock guard(m_dataLock);

	for (auto it = begin(m_data); it != end(m_data);) {
		const auto &insertedAt = it->second.receivedAt;
		if (insertedAt.isElapsed(timeout.totalMicroseconds()))
			it = m_data.erase(it);
		else
			++it;
	}
}
