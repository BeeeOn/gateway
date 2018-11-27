#include <Poco/AutoPtr.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/NodeFilter.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/NumberParser.h>

#include "belkin/BelkinWemoSwitch.h"
#include "net/HTTPEntireResponse.h"
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

BelkinWemoSwitch::BelkinWemoSwitch(
		const SocketAddress& address,
		const Timespan &httpTimeout):
	BelkinWemoStandaloneDevice(
		URI("http://" + address.toString() + "/upnp/control/basicevent1"),
		httpTimeout)
{
}

BelkinWemoSwitch::~BelkinWemoSwitch()
{
}

BelkinWemoSwitch::Ptr BelkinWemoSwitch::buildDevice(const SocketAddress& address, const Timespan& timeout)
{
	return new BelkinWemoSwitch(address, timeout);
}

bool BelkinWemoSwitch::requestModifyState(const ModuleID& moduleID, const double value)
{
	if (moduleID != BELKIN_SWITCH_MODULE_ID)
		return false;

	if (value)
		return requestModifyBinaryState("BinaryState", "BinaryState", BELKIN_SWITCH_STATE_ON);
	else
		return requestModifyBinaryState("BinaryState", "BinaryState", BELKIN_SWITCH_STATE_OFF);
}

SensorData BelkinWemoSwitch::requestState()
{
	HTTPEntireResponse response = requestBinaryState();

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
