#include <Poco/AutoPtr.h>
#include <Poco/NumberParser.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>

#include "belkin/BelkinWemoDimmer.h"
#include "net/HTTPEntireResponse.h"
#include "util/SecureXmlParser.h"

#define BELKIN_DIMMER_NAME "Dimmer"
#define STATE_ON 1
#define STATE_OFF 0
#define ON_OFF_MODULE_ID 0
#define DIMMER_MODULE_ID 1

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::Net;
using namespace Poco::XML;
using namespace std;

static list<ModuleType> DIMMER_MODULE_TYPES = {
	ModuleType(ModuleType::Type::TYPE_ON_OFF, {ModuleType::Attribute::ATTR_CONTROLLABLE}),
	ModuleType(ModuleType::Type::TYPE_BRIGHTNESS, {ModuleType::Attribute::ATTR_CONTROLLABLE})
};

BelkinWemoDimmer::BelkinWemoDimmer(
		const SocketAddress& address,
		const Timespan &httpTimeout,
		const RefreshTime &refresh):
	BelkinWemoStandaloneDevice(
		URI("http://" + address.toString() + "/upnp/control/basicevent1"),
		httpTimeout,
		refresh)
{
}

bool BelkinWemoDimmer::requestModifyState(const ModuleID& moduleID, const double value)
{
	switch (moduleID.value()) {
	case ON_OFF_MODULE_ID:
		if (value)
			return requestModifyBinaryState("BinaryState", "BinaryState", STATE_ON);
		else
			return requestModifyBinaryState("BinaryState", "BinaryState", STATE_OFF);
	case DIMMER_MODULE_ID:
		return requestModifyBinaryState("brightness", "Brightness", value);
	default:
		logger().warning("invalid module ID: " + to_string(moduleID.value()), __FILE__, __LINE__);
		return false;
	}
}

SensorData BelkinWemoDimmer::requestState()
{
	HTTPEntireResponse response = requestBinaryState();

	SensorData data;
	data.setDeviceID(m_deviceId);

	SecureXmlParser parser;
	AutoPtr<Document> xmlDoc = parser.parse(response.getBody());
	NodeIterator iterator(xmlDoc, NodeFilter::SHOW_ALL);

	Node* xmlNode = findNode(iterator, "BinaryState");
	if (xmlNode->nodeValue() == "1")
		data.insertValue(SensorValue(ON_OFF_MODULE_ID, STATE_ON));
	else
		data.insertValue(SensorValue(ON_OFF_MODULE_ID, STATE_OFF));

	xmlNode = findNode(iterator, "brightness");
	data.insertValue(SensorValue(DIMMER_MODULE_ID, NumberParser::parse(xmlNode->nodeValue())));

	return data;
}

list<ModuleType> BelkinWemoDimmer::moduleTypes() const
{
	return DIMMER_MODULE_TYPES;
}

string BelkinWemoDimmer::name() const
{
	return BELKIN_DIMMER_NAME;
}

bool BelkinWemoDimmer::operator==(const BelkinWemoDimmer& bw) const
{
	return bw.deviceID() == m_deviceId;
}
