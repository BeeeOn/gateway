#include <Poco/Exception.h>
#include <Poco/SharedPtr.h>

#include "server/GWSConnector.h"

namespace BeeeOn {

/**
 * @brief MockGWSConnector is intended for testing and allows
 * to deliver fake messages via its method MockGWSConnector::receive().
 */
class MockGWSConnector : public GWSConnector {
public:
	typedef Poco::SharedPtr<MockGWSConnector> Ptr;

	/**
	 * @brief Set exception to be thrown from send().
	 */
	void setSendException(const Poco::SharedPtr<Poco::Exception> e);

	/**
	 * @brief Execute event GWSListener::onSend(). If a send
	 * exception is set, no event is generated and the exception
	 * is thrown.
	 */
	void send(const GWMessage::Ptr message) override;

	/**
	 * @brief Execute the appropriate event, one of
	 * GWSListener::onRequest(), GWSListener::onResponse(),
	 * GWSListener::onAck(), GWSListener::onOther().
	 */
	void receive(const GWMessage::Ptr message);

private:
	Poco::SharedPtr<Poco::Exception> m_sendException;
};

}
