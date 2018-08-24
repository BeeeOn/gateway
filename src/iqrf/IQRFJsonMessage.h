#pragma once

#include <string>

#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/JSON/Object.h>

namespace BeeeOn {

class IQRFJsonMessage {
public:
	typedef Poco::SharedPtr<IQRFJsonMessage> Ptr;

	IQRFJsonMessage();
	virtual ~IQRFJsonMessage();

	/**
	 * @returns request/response from received JSON string.
	 */
	static IQRFJsonMessage::Ptr parse(const std::string &data);

	/**
	 * @returns JSON string with required data.
	 */
	virtual std::string toString() =0;

	void setMessageID(const std::string &id);
	std::string messageID() const;

	void setTimeout(const Poco::Timespan &timeout);
	Poco::Timespan timeout() const;

private:
	std::string m_id;
	Poco::Timespan m_timeout;
};

}
