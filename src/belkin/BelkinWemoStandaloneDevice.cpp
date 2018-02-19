#include <Poco/AutoPtr.h>
#include <Poco/NumberParser.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/SAX/AttributesImpl.h>
#include <Poco/XML/XMLWriter.h>

#include "belkin/BelkinWemoStandaloneDevice.h"
#include "model/DevicePrefix.h"
#include "net/HTTPUtil.h"
#include "net/SOAPMessage.h"
#include "util/SecureXmlParser.h"

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Net;
using namespace Poco::XML;
using namespace std;

BelkinWemoStandaloneDevice::BelkinWemoStandaloneDevice(const URI& uri):
	m_uri(uri)
{
}

MACAddress BelkinWemoStandaloneDevice::requestMacAddr() const
{
	HTTPRequest request;

	SOAPMessage msg;
	msg.setAction("\"urn:Belkin:service:basicevent:1#GetMacAddr\"");

	AttributesImpl attrs;
	XMLWriter& writer = msg.bodyWriter();

	attrs.addAttribute(
		"",
		"xmlns:u",
		"xmlns",
		"string",
		"urn:Belkin:service:basicevent:1");
	writer.startElement(
		"",
		"u:GetMacAddr",
		"GetMacAddr",
		attrs);

	writer.endElement(
		"",
		"u:GetMacAddr",
		"GetMacAddr");

	msg.prepare(request);

	HTTPEntireResponse response = HTTPUtil::makeRequest(
		request, m_uri, msg.toString(), m_httpTimeout);

	SecureXmlParser parser;
	AutoPtr<Document> xmlDoc = parser.parse(response.getBody());
	NodeIterator iterator(xmlDoc, NodeFilter::SHOW_ALL);
	Node* xmlNode = findNode(iterator, "MacAddr");

	return MACAddress(NumberParser::parseHex64(xmlNode->nodeValue()));
}


HTTPEntireResponse BelkinWemoStandaloneDevice::requestBinaryState() const
{
	HTTPRequest request;

	SOAPMessage msg;
	msg.setAction("\"urn:Belkin:service:basicevent:1#GetBinaryState\"");

	AttributesImpl attrs;
	XMLWriter& writer = msg.bodyWriter();

	attrs.addAttribute(
		"",
		"xmlns:u",
		"xmlns",
		"string",
		"urn:Belkin:service:basicevent:1");
	writer.startElement(
		"",
		"u:GetBinaryState",
		"GetBinaryState",
		attrs);

	writer.startElement(
		"",
		"BinaryState",
		"BinaryState");

	writer.endElement(
		"",
		"BinaryState",
		"BinaryState");
	writer.endElement(
		"",
		"u:GetBinaryState",
		"GetBinaryState");

	msg.prepare(request);

	return HTTPUtil::makeRequest(
		request, m_uri, msg.toString(), m_httpTimeout);
}


bool BelkinWemoStandaloneDevice::requestModifyBinaryState(
	const string& setModuleName, const string& getModuleName, const int value) const
{
	HTTPRequest request;

	SOAPMessage msg;
	msg.setAction("\"urn:Belkin:service:basicevent:1#SetBinaryState\"");

	AttributesImpl attrs;
	XMLWriter& writer = msg.bodyWriter();

	attrs.addAttribute(
		"",
		"xmlns:u",
		"xmlns",
		"string",
		"urn:Belkin:service:basicevent:1");
	writer.startElement(
		"",
		"u:SetBinaryState",
		"SetBinaryState",
		attrs);

	writer.startElement(
		"",
		setModuleName,
		setModuleName);
	writer.characters(to_string(value));

	writer.endElement(
		"",
		setModuleName,
		setModuleName);
	writer.endElement(
		"",
		"u:SetBinaryState",
		"SetBinaryState");

	msg.prepare(request);

	HTTPEntireResponse response = HTTPUtil::makeRequest(
		request, m_uri, msg.toString(), m_httpTimeout);

	SecureXmlParser parser;
	AutoPtr<Document> xmlDoc = parser.parse(response.getBody());
	NodeIterator iterator(xmlDoc, NodeFilter::SHOW_ALL);
	Node* xmlNode = findNode(iterator, getModuleName);

	return xmlNode->nodeValue() == to_string(value);
}

SocketAddress BelkinWemoStandaloneDevice::address() const
{
	return SocketAddress(m_uri.getHost(), m_uri.getPort());
}


void BelkinWemoStandaloneDevice::setAddress(const SocketAddress& address)
{
	m_uri.setHost(address.host().toString());
	m_uri.setPort(address.port());
}

void BelkinWemoStandaloneDevice::buildDeviceID()
{
	m_deviceId = DeviceID(DevicePrefix::PREFIX_BELKIN_WEMO, requestMacAddr());
}
