#pragma once

#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>

#include "net/MqttMessage.h"

namespace BeeeOn {

class MqttClient {
public:
	typedef Poco::SharedPtr<MqttClient> Ptr;

	virtual ~MqttClient();

	/**
	 * @brief Publish a message on the topic included in the message.
	 */
	virtual void publish(const MqttMessage &msg) = 0;

	/**
	 * Waiting for a new message according to given timeout.
	 * Returns message, if there is any. Otherwise it waits
	 * for a new message or timeout exception.
	 *
	 * This method should not be called by multiple threads,
	 * received message could be given to only one thread.
	 *
	 * Timeout:
	 *  - 0 - non-blocking
	 *  - negative - blocking
	 *  - positive blocking with timeout
	 */
	virtual MqttMessage receive(const Poco::Timespan &timeout) = 0;
};

}
