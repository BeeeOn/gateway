#include <Poco/AutoPtr.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/Exception.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>
#include <Poco/SAX/AttributesImpl.h>
#include <Poco/StreamCopier.h>
#include <Poco/XML/XMLWriter.h>

#include "belkin/BelkinWemoSwitch.h"
#include "model/DevicePrefix.h"
#include "net/SOAPMessage.h"
#include "util/SecureXmlParser.h"

#define BELKIN_SWITCH_NAME "Switch F7C027fr"
#define BELKIN_SWITCH_STATE_ON 1
#define BELKIN_SWITCH_STATE_OFF 0
#define BELKIN_SWITCH_MODULE_ID ModuleID(0)

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Net;
using namespace Poco::XML;
using namespace std;

BelkinWemoSwitch::BelkinWemoSwitch(const SocketAddress& address):
	m_uri("http://" + address.toString() + "/upnp/control/basicevent1")
{
}

BelkinWemoSwitch::BelkinWemoSwitch()
{
}

BelkinWemoSwitch::~BelkinWemoSwitch()
{
}

BelkinWemoSwitch::Ptr BelkinWemoSwitch::buildDevice(const SocketAddress& address, const Timespan& timeout)
{
	BelkinWemoSwitch::Ptr device = new BelkinWemoSwitch(address);
	device->m_httpTimeout = timeout;
	device->buildDeviceID();
	return device;
}

void BelkinWemoSwitch::buildDeviceID()
{
	m_deviceId = DeviceID(DevicePrefix::PREFIX_BELKIN_WEMO, requestMacAddr());
}

bool BelkinWemoSwitch::requestModifyState(const ModuleID& moduleID, const bool value)
{
	if (moduleID != BELKIN_SWITCH_MODULE_ID)
		return false;

	if (value)
		return turnOn();
	else
		return turnOff();
}

bool BelkinWemoSwitch::turnOn() const
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
		"BinaryState",
		"BinaryState");
	writer.characters(to_string(BELKIN_SWITCH_STATE_ON));

	writer.endElement(
		"",
		"BinaryState",
		"BinaryState");
	writer.endElement(
		"",
		"u:SetBinaryState",
		"SetBinaryState");

	msg.prepare(request);

	HTTPEntireResponse response = sendHTTPRequest(request, msg.toString(), m_httpTimeout);

	SecureXmlParser parser;
	AutoPtr<Document> xmlDoc = parser.parse(response.getBody());
	NodeIterator iterator(xmlDoc, NodeFilter::SHOW_ALL);
	Node* xmlNode = findNode(iterator, "BinaryState");

	return xmlNode->nodeValue() == "1";
}

bool BelkinWemoSwitch::turnOff() const
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
		"BinaryState",
		"BinaryState");
	writer.characters(to_string(BELKIN_SWITCH_STATE_OFF));

	writer.endElement(
		"",
		"BinaryState",
		"BinaryState");
	writer.endElement(
		"",
		"u:SetBinaryState",
		"SetBinaryState");

	msg.prepare(request);

	HTTPEntireResponse response = sendHTTPRequest(request, msg.toString(), m_httpTimeout);

	SecureXmlParser parser;
	AutoPtr<Document> xmlDoc = parser.parse(response.getBody());
	NodeIterator iterator(xmlDoc, NodeFilter::SHOW_ALL);
	Node* xmlNode = findNode(iterator, "BinaryState");

	return xmlNode->nodeValue() == "0";
}

SensorData BelkinWemoSwitch::requestState() const
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

	HTTPEntireResponse response = sendHTTPRequest(request, msg.toString(), m_httpTimeout);

	SecureXmlParser parser;
	AutoPtr<Document> xmlDoc = parser.parse(response.getBody());
	NodeIterator iterator(xmlDoc, NodeFilter::SHOW_ALL);
	Node* xmlNode = findNode(iterator, "BinaryState");

	SensorData data;
	data.setDeviceID(m_deviceId);
	if (xmlNode->nodeValue() == "1")
		data.insertValue(SensorValue(BELKIN_SWITCH_MODULE_ID, BELKIN_SWITCH_STATE_ON));
	else
		data.insertValue(SensorValue(BELKIN_SWITCH_MODULE_ID, BELKIN_SWITCH_STATE_OFF));

	return data;
}

MACAddress BelkinWemoSwitch::requestMacAddr()
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

	HTTPEntireResponse response = sendHTTPRequest(request, msg.toString(), m_httpTimeout);

	SecureXmlParser parser;
	AutoPtr<Document> xmlDoc = parser.parse(response.getBody());
	NodeIterator iterator(xmlDoc, NodeFilter::SHOW_ALL);
	Node* xmlNode = findNode(iterator, "MacAddr");

	return MACAddress(NumberParser::parseHex64(xmlNode->nodeValue()));
}

Node* BelkinWemoSwitch::findNode(NodeIterator& iterator, const string& name) const
{
	Node* xmlNode = iterator.nextNode();

	while (xmlNode) {
		if (xmlNode->nodeName() == name) {
			xmlNode = iterator.nextNode();
			return xmlNode;
		}

		xmlNode = iterator.nextNode();
	}

	throw SyntaxException("node " + name + " in XML message from belkin device not found");
}

DeviceID BelkinWemoSwitch::deviceID() const
{
	return m_deviceId;
}

SocketAddress BelkinWemoSwitch::address() const
{
	return SocketAddress(m_uri.getHost(), m_uri.getPort());
}

void BelkinWemoSwitch::setAddress(const SocketAddress& address)
{
	m_uri = URI("http://" + address.toString() + "/upnp/control/basicevent1");
}

list<ModuleType> BelkinWemoSwitch::moduleTypes() const
{
	std::list<ModuleType> moduleTypes;
	moduleTypes.push_back(
		ModuleType(
			ModuleType::Type::TYPE_ON_OFF, {ModuleType::Attribute::ATTR_CONTROLLABLE}
		)
	);

	return moduleTypes;
}

string BelkinWemoSwitch::name() const
{
	return BELKIN_SWITCH_NAME;
}

bool BelkinWemoSwitch::operator==(const BelkinWemoSwitch& bws) const
{
	return bws.deviceID() == m_deviceId;
}

FastMutex& BelkinWemoSwitch::lock()
{
	return m_lock;
}

HTTPEntireResponse BelkinWemoSwitch::sendHTTPRequest(HTTPRequest& request, const string& msg, const Timespan& timeout) const
{
	HTTPClientSession http;
	HTTPEntireResponse response;

	http.setHost(m_uri.getHost());
	http.setPort(m_uri.getPort());
	http.setTimeout(timeout);

	request.setURI(m_uri.toString());

	http.sendRequest(request) << msg;
	istream& input = http.receiveResponse(response);
	response.readBody(input);

	return response;
}
