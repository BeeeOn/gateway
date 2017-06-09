#include <Poco/SAX/AttributesImpl.h>

#include "net/SOAPMessage.h"

using namespace BeeeOn;
using namespace Poco::Net;
using namespace Poco::XML;
using namespace std;

SOAPMessage::SOAPMessage():
	m_writer(m_stream, XMLWriter::WRITE_XML_DECLARATION),
	m_writerDone(false)
{
	AttributesImpl attrs;
	m_writer.startDocument();

	attrs.addAttribute(
		"",
		"xmlns:s",
		"xmlns",
		"string",
		"http://schemas.xmlsoap.org/soap/envelope/");
	attrs.addAttribute(
		"",
		"s:encodingStyle",
		"encodingStyle",
		"string",
		"http://schemas.xmlsoap.org/soap/encoding/");
	m_writer.startElement(
		"",
		"s:Envelope",
		"Envelope",
		attrs);

	m_writer.startElement(
		"",
		"s:Body",
		"Body");
}

SOAPMessage::~SOAPMessage()
{
}

void SOAPMessage::setAction(const string& action)
{
	m_action = action;
}

XMLWriter& SOAPMessage::bodyWriter()
{
	return m_writer;
}

void SOAPMessage::prepare(HTTPRequest& request)
{
	if (!m_writerDone) {
		m_writer.endElement(
			"",
			"s:Body",
			"Body");
		m_writer.endElement(
			"",
			"s:Envelope",
			"Envelope");
		m_writer.endDocument();
	}

	m_writerDone = true;

	request.setMethod(HTTPRequest::HTTP_POST);
	request.set("SOAPACTION", m_action);
	request.setContentType("text/xml; charset=\"utf-8\"");
	request.setContentLength(m_stream.str().length());
}

string SOAPMessage::toString() const
{
	return m_stream.str();
}

