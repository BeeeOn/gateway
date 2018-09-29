#pragma once

#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>

#include "gwmessage/GWMessage.h"
#include "server/GWSListener.h"
#include "util/EventSource.h"

namespace BeeeOn {

/**
 * @brief GWSConnector is an abstract class that defines an API
 * for communication with a remote server. It keeps the connection
 * alive and allows to send and receive messages.
 *
 * All received messages and other events are reported via the
 * GWSListener interface. Sending is done by the provided method
 * GWSConnector::send().
 */
class GWSConnector {
public:
	typedef Poco::SharedPtr<GWSConnector> Ptr;

	virtual ~GWSConnector();

	/**
	 * @brief Send the given message to the remote server.
	 * The actual sending operation might be delayed and thus
	 * the result of this call might be just appending the
	 * message into an output queue.
	 */
	virtual void send(const GWMessage::Ptr message) = 0;

	/**
	 * @brief Register a GWSListener instance that would receive
	 * events related to the communication.
	 */
	void addListener(GWSListener::Ptr listener);

	/**
	 * @brief Remove all registered listeners.
	 */
	void clearListeners();

	/**
	 * @brief Configure an AsyncExecutor instance that would be
	 * used for GWSListener events delivery.
	 */
	void setEventsExecutor(AsyncExecutor::Ptr executor);

protected:
	template <typename Event, typename Method>
	void fireEvent(const Event &e, const Method &m)
	{
		m_eventSource.fireEvent(e, m);
	}

	void fireReceived(const GWMessage::Ptr message);

private:
	EventSource<GWSListener> m_eventSource;
};

}
