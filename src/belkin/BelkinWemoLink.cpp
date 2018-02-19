#include <Poco/Exception.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/SAX/AttributesImpl.h>
#include <Poco/XML/XMLWriter.h>

#include "belkin/BelkinWemoDevice.h"
#include "belkin/BelkinWemoLink.h"
#include "net/HTTPEntireResponse.h"
#include "net/HTTPUtil.h"
#include "net/SOAPMessage.h"
#include "util/SecureXmlParser.h"

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Net;
using namespace Poco::XML;
using namespace std;

BelkinWemoLink::BelkinWemoLink(const Poco::Net::SocketAddress& address):
	m_address(address),
	m_countOfBulbs(0)
{
}

BelkinWemoLink::Ptr BelkinWemoLink::buildDevice(const Poco::Net::SocketAddress& address,
	const Timespan& timeout)
{
	BelkinWemoLink::Ptr link = new BelkinWemoLink(address);
	link->m_httpTimeout = timeout;
	link->requestDeviceInfo();

	return link;
}

void BelkinWemoLink::requestDeviceInfo()
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

	URI uri("http://" + m_address.toString() + "/upnp/control/basicevent1");
	HTTPEntireResponse response = HTTPUtil::makeRequest(
		request, uri, msg.toString(), m_httpTimeout);

	SecureXmlParser parser;
	AutoPtr<Document> xmlDoc = parser.parse(response.getBody());
	NodeIterator iterator(xmlDoc, NodeFilter::SHOW_ALL);

	Node* xmlNode = BelkinWemoDevice::findNode(iterator, "MacAddr");
	m_macAddr = MACAddress(NumberParser::parseHex64(xmlNode->nodeValue()));

	xmlNode = BelkinWemoDevice::findNode(iterator, "PluginUDN");
	m_udn = xmlNode->nodeValue();
}

std::list<BelkinWemoLink::BulbID> BelkinWemoLink::requestDeviceList()
{
	HTTPRequest request;

	SOAPMessage msg;
	msg.setAction("\"urn:Belkin:service:bridge:1#GetEndDevices\"");

	AttributesImpl attrs;
	XMLWriter& writer = msg.bodyWriter();

	attrs.addAttribute(
		"",
		"xmlns:u",
		"xmlns",
		"string",
		"urn:Belkin:service:bridge:1");
	writer.startElement(
		"",
		"u:GetEndDevices",
		"GetEndDevices",
		attrs);

	writer.startElement(
		"",
		"DevUDN",
		"DevUDN");
	writer.characters(m_udn);
	writer.endElement(
		"",
		"DevUDN",
		"DevUDN");

	writer.startElement(
		"",
		"ReqListType",
		"ReqListType");
	writer.characters("PAIRED_LIST");
	writer.endElement(
		"",
		"ReqListType",
		"ReqListType");

	writer.endElement(
		"",
		"u:GetEndDevices",
		"GetEndDevices");

	msg.prepare(request);

	URI uri("http://" + m_address.toString() + "/upnp/control/bridge1");
	HTTPEntireResponse response = HTTPUtil::makeRequest(
		request, uri, msg.toString(), m_httpTimeout);

	SecureXmlParser parser;
	AutoPtr<Document> xmlDoc = parser.parse(response.getBody());
	NodeIterator iterator(xmlDoc, NodeFilter::SHOW_ALL);
	Node* xmlNode = BelkinWemoDevice::findNode(iterator, "DeviceLists");

	xmlDoc = parser.parse(xmlNode->nodeValue());
	iterator = NodeIterator(xmlDoc, NodeFilter::SHOW_ALL);
	std::list<Node*> list = BelkinWemoDevice::findNodes(iterator, "DeviceID");;

	std::list<BelkinWemoLink::BulbID> bulbIDs;
	for (auto bulbID : list)
		bulbIDs.push_back(NumberParser::parseHex64(bulbID->nodeValue()));

	return bulbIDs;
}

bool BelkinWemoLink::requestModifyState(const BelkinWemoLink::BulbID bulbID,
	const int capability, const string& value)
{
	HTTPRequest request;

	SOAPMessage msg;
	msg.setAction("\"urn:Belkin:service:bridge:1#SetDeviceStatus\"");

	AttributesImpl attrs;
	XMLWriter& writer = msg.bodyWriter();

	attrs.addAttribute(
		"",
		"xmlns:u",
		"xmlns",
		"string",
		"urn:Belkin:service:bridge:1");
	writer.startElement(
		"",
		"u:SetDeviceStatus",
		"SetDeviceStatus",
		attrs);

	writer.startElement(
		"",
		"DeviceStatusList",
		"DeviceStatusList");
	writer.characters(
		"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
		"<DeviceStatus>"
		"<IsGroupAction>NO</IsGroupAction>"
		"<DeviceID available=\"YES\">" + NumberFormatter::formatHex(bulbID) + "</DeviceID>"
		"<CapabilityID>" + to_string(capability) + "</CapabilityID>"
		"<CapabilityValue>" + value + "</CapabilityValue>"
		"</DeviceStatus>");
	writer.endElement(
		"",
		"DeviceStatusList",
		"DeviceStatusList");

	writer.endElement(
		"",
		"u:SetDeviceStatus",
		"SetDeviceStatus");

	msg.prepare(request);

	URI uri("http://" + m_address.toString() + "/upnp/control/bridge1");
	HTTPEntireResponse response = HTTPUtil::makeRequest(
		request, uri, msg.toString(), m_httpTimeout);

	SecureXmlParser parser;
	AutoPtr<Document> xmlDoc = parser.parse(response.getBody());
	NodeIterator iterator(xmlDoc, NodeFilter::SHOW_ALL);
	Node* xmlNode = BelkinWemoDevice::findNode(iterator, "ErrorDeviceIDs");

	if (xmlNode)
		return false;
	else
		return true;
}

string BelkinWemoLink::requestDeviceState(const BelkinWemoLink::BulbID bulbID)
{
	HTTPRequest request;

	SOAPMessage msg;
	msg.setAction("\"urn:Belkin:service:bridge:1#GetDeviceStatus\"");

	AttributesImpl attrs;
	XMLWriter& writer = msg.bodyWriter();

	attrs.addAttribute(
		"",
		"xmlns:u",
		"xmlns",
		"string",
		"urn:Belkin:service:bridge:1");
	writer.startElement(
		"",
		"u:GetDeviceStatus",
		"GetDeviceStatus",
		attrs);

	writer.startElement(
		"",
		"DeviceIDs",
		"DeviceIDs");
	writer.characters(NumberFormatter::formatHex(bulbID));
	writer.endElement(
		"",
		"DeviceIDs",
		"DeviceIDs");

	writer.endElement(
		"",
		"u:GetDeviceStatus",
		"GetDeviceStatus");

	msg.prepare(request);

	URI uri("http://" + m_address.toString() + "/upnp/control/bridge1");
	HTTPEntireResponse response = HTTPUtil::makeRequest(
		request, uri, msg.toString(), m_httpTimeout);

	return response.getBody();
}

void BelkinWemoLink::incrementCountOfBulbs()
{
	ScopedLock<FastMutex> guard(m_lockCountOfBulbs);

	m_countOfBulbs++;
}

void BelkinWemoLink::decrementCountOfBulbs()
{
	ScopedLock<FastMutex> guard(m_lockCountOfBulbs);

	if (m_countOfBulbs == 0)
		throw IllegalStateException("count of bulbs can not be negative");

	m_countOfBulbs--;
}

uint32_t BelkinWemoLink::countOfBulbs()
{
	ScopedLock<FastMutex> guard(m_lockCountOfBulbs);

	return m_countOfBulbs;
}

SocketAddress BelkinWemoLink::address() const
{
	return m_address;
}

void BelkinWemoLink::setAddress(const SocketAddress& address)
{
	m_address = address;
}

MACAddress BelkinWemoLink::macAddress() const
{
	return m_macAddr;
}

FastMutex& BelkinWemoLink::lock()
{
	return m_lock;
}

bool BelkinWemoLink::operator==(const BelkinWemoLink& bwl) const
{
	return m_macAddr == bwl.m_macAddr;
}
