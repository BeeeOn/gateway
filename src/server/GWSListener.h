#pragma once

#include <string>

#include <Poco/SharedPtr.h>
#include <Poco/Net/SocketAddress.h>

#include "gwmessage/GWAck.h"
#include "gwmessage/GWRequest.h"
#include "gwmessage/GWResponse.h"

namespace BeeeOn {

/**
 * @brief GWSListener provides an interface for delivering of
 * events and messages related to communication with the remote
 * gateway server.
 */
class GWSListener {
public:
	typedef Poco::SharedPtr<GWSListener> Ptr;

	struct Address {
		const std::string host;
		const int port;

		std::string toString() const;
	};

	virtual ~GWSListener();

	/**
	 * @brief Fired when the connection to the remote server is
	 * successfully created and it is possible to exchange messages.
	 */
	virtual void onConnected(const Address &address);

	/**
	 * @brief Fired when the connection to the remote server is
	 * considered broken or when it is disconnected on a request.
	 */
	virtual void onDisconnected(const Address &address);

	/**
	 * @brief When a request is received, this event is fired.
	 */
	virtual void onRequest(const GWRequest::Ptr request);

	/**
	 * @brief When a response is received, this event is fired.
	 */
	virtual void onResponse(const GWResponse::Ptr response);

	/**
	 * @brief When an ack is received, this event is fired.
	 */
	virtual void onAck(const GWAck::Ptr ack);

	/**
	 * @brief When a message other then request, response or ack
	 * is received, this event is fired.
	 */
	virtual void onOther(const GWMessage::Ptr other);

	/**
	 * @brief Fire when a message is about to be sent to the server.
	 * After the send is successful (no network failure), the GWSListener::onSend()
	 * event would be generated as well.
	 */
	virtual void onTrySend(const GWMessage::Ptr message);

	/**
	 * @brief Fire when a message is being sent to the server.
	 * There might be a delay between putting a messing into
	 * an output queue and the actual sending process. This event
	 * allows to track such delay.
	 */
	virtual void onSent(const GWMessage::Ptr message);
};

}
