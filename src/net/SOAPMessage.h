#ifndef BEEEON_SOAP_MESSAGE_H
#define BEEEON_SOAP_MESSAGE_H

#include <string>
#include <sstream>

#include <Poco/Net/HTTPRequest.h>
#include <Poco/XML/XMLWriter.h>

namespace BeeeOn {

class SOAPMessage {
public:
	SOAPMessage();
	~SOAPMessage();

	void setAction(const std::string& action);
	Poco::XML::XMLWriter& bodyWriter();
	void prepare(Poco::Net::HTTPRequest& request);

	std::string toString() const;

private:
	std::string m_action;
	std::stringstream m_stream;
	Poco::XML::XMLWriter m_writer;
	bool m_writerDone;
};

}

#endif
