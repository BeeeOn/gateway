#include <cmath>

#include <Poco/AutoPtr.h>
#include <Poco/Exception.h>
#include <Poco/NumberParser.h>
#include <Poco/RegularExpression.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>

#include "belkin/BelkinWemoBulb.h"
#include "model/DevicePrefix.h"
#include "util/SecureXmlParser.h"

#define BELKIN_LED_NAME "Led Light Bulb"
#define LED_LIGHT_DIMMER_CAPABILITY 10008
#define LED_LIGHT_ON_OFF_CAPABILITY 10006
#define LED_LIGHT_DIMMER_MODULE_ID 1
#define LED_LIGHT_ON_OFF_MODULE_ID 0
#define LED_LIGHT_OFF 0
#define LED_LIGHT_ON 1
#define MAX_DIM 255

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::XML;
using namespace std;

static list<ModuleType> BULB_MODULE_TYPES = {
	ModuleType(ModuleType::Type::TYPE_ON_OFF, {ModuleType::Attribute::ATTR_CONTROLLABLE}),
	ModuleType(ModuleType::Type::TYPE_BRIGHTNESS, {ModuleType::Attribute::ATTR_CONTROLLABLE})
};

BelkinWemoBulb::BelkinWemoBulb(
		const BelkinWemoLink::BulbID bulbId,
		const BelkinWemoLink::Ptr link,
		const RefreshTime &refresh):
	BelkinWemoDevice(buildDeviceID(bulbId), refresh),
	m_bulbId(bulbId),
	m_link(link)
{
	m_link->incrementCountOfBulbs();
}

DeviceID BelkinWemoBulb::buildDeviceID(const BelkinWemoLink::BulbID &id)
{
	return {DevicePrefix::PREFIX_BELKIN_WEMO, id & DeviceID::IDENT_MASK};
}

BelkinWemoBulb::~BelkinWemoBulb()
{
	try {
		m_link->decrementCountOfBulbs();
	}
	catch (IllegalStateException& e) {
		logger().log(e, __FILE__, __LINE__);
	}
}

bool BelkinWemoBulb::requestModifyState(const ModuleID& moduleID, const double value)
{
	switch (moduleID.value()) {
	case LED_LIGHT_ON_OFF_MODULE_ID:
		if (value)
			return m_link->requestModifyState(m_bulbId, LED_LIGHT_ON_OFF_CAPABILITY, to_string(LED_LIGHT_ON));
		else
			return m_link->requestModifyState(m_bulbId, LED_LIGHT_ON_OFF_CAPABILITY, to_string(LED_LIGHT_OFF));
	case LED_LIGHT_DIMMER_MODULE_ID:
		return m_link->requestModifyState(m_bulbId, LED_LIGHT_DIMMER_CAPABILITY,
			to_string(dimFromPercentage(value)) + ":0");
	default:
		logger().warning("unknown operation", __FILE__, __LINE__);
		return false;
	}
}

SensorData BelkinWemoBulb::requestState()
{
	string body = m_link->requestDeviceState(m_bulbId);

	SecureXmlParser parser;
	AutoPtr<Document> xmlDoc = parser.parse(body);
	NodeIterator iterator(xmlDoc, NodeFilter::SHOW_ALL);
	Node* xmlNode = findNode(iterator, "DeviceStatusList");

	xmlDoc = parser.parse(xmlNode->nodeValue());
	iterator = NodeIterator(xmlDoc, NodeFilter::SHOW_ALL);
	xmlNode = findNode(iterator, "CapabilityValue");

	RegularExpression::MatchVec matches;
	RegularExpression re("(0|1),([0-9]+):0,,,");
	if (re.match(xmlNode->nodeValue(), 0, matches) == 0)
		throw SyntaxException("wrong syntax of CapabilityValue element " + xmlNode->nodeValue());

	SensorData data;
	data.setDeviceID(m_deviceId);

	int dim = NumberParser::parse(
		xmlNode->nodeValue().substr(matches[2].offset, matches[2].length));
	data.insertValue(SensorValue(LED_LIGHT_DIMMER_MODULE_ID, dimToPercentage(dim)));

	if (xmlNode->nodeValue().substr(matches[1].offset, matches[1].length) == "1")
		data.insertValue(SensorValue(LED_LIGHT_ON_OFF_MODULE_ID, LED_LIGHT_ON));
	else
		data.insertValue(SensorValue(LED_LIGHT_ON_OFF_MODULE_ID, LED_LIGHT_OFF));

	return data;
}

list<ModuleType> BelkinWemoBulb::moduleTypes() const
{
	return BULB_MODULE_TYPES;
}

string BelkinWemoBulb::name() const
{
	return BELKIN_LED_NAME;
}

FastMutex& BelkinWemoBulb::lock()
{
	return m_link->lock();
}

BelkinWemoLink::Ptr BelkinWemoBulb::link() const
{
	return m_link;
}

int BelkinWemoBulb::dimToPercentage(const double value)
{
	if (value < 0 || value > MAX_DIM)
		throw IllegalStateException("value is out of range");

	return round((value / MAX_DIM) * 100.0);
}

int BelkinWemoBulb::dimFromPercentage(const double percents)
{
	if (percents < 0 || percents > 100)
		throw IllegalStateException("percents are out of range");

	return round((percents * MAX_DIM) / 100.0);
}
