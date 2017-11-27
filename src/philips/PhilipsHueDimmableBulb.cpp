#include <Poco/Logger.h>
#include <Poco/NumberParser.h>
#include <Poco/JSON/Object.h>

#include "model/DevicePrefix.h"
#include "philips/PhilipsHueDimmableBulb.h"
#include "util/JsonUtil.h"

#define PHILIPS_BULB_NAME "Dimmable Light Bulb"
#define LED_LIGHT_DIMMER_MODULE_ID 1
#define LED_LIGHT_ON_OFF_MODULE_ID 0

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::JSON;
using namespace std;

static list<ModuleType> BULB_MODULE_TYPES = {
	ModuleType(ModuleType::Type::TYPE_ON_OFF,
		{ModuleType::Attribute::ATTR_CONTROLLABLE}),
	ModuleType(ModuleType::Type::TYPE_BRIGHTNESS,
		{ModuleType::Attribute::ATTR_CONTROLLABLE})
};

PhilipsHueDimmableBulb::PhilipsHueDimmableBulb(const uint32_t ordinalNumber,
	const PhilipsHueBridge::BulbID bulbId, const PhilipsHueBridge::Ptr bridge):
	PhilipsHueBulb(ordinalNumber, bulbId, bridge)
{
}

bool PhilipsHueDimmableBulb::requestModifyState(const ModuleID& moduleID, const double value)
{
	switch (moduleID.value()) {
	case LED_LIGHT_ON_OFF_MODULE_ID:
		return m_bridge->requestModifyState(
			m_ordinalNumber, "on", decodeOnOffValue(value));
	case LED_LIGHT_DIMMER_MODULE_ID:
		return m_bridge->requestModifyState(
			m_ordinalNumber, "bri", dimFromPercentage(value));
	default:
		logger().warning("unknown operation", __FILE__, __LINE__);
		return false;
	}
}

SensorData PhilipsHueDimmableBulb::requestState()
{
	string response = m_bridge->requestDeviceState(m_ordinalNumber);

	Object::Ptr object = JsonUtil::parse(response);
	Object::Ptr state = object->getObject("state");

	if (state->getValue<bool>("reachable") == false)
		throw InvalidArgumentException(
			"bulb " + m_deviceID.toString() + " is unreachable");

	SensorData data;
	data.setDeviceID(m_deviceID);

	uint32_t bri = NumberParser::parse(state->getValue<string>("bri"));
	data.insertValue(SensorValue(LED_LIGHT_DIMMER_MODULE_ID, dimToPercentage(bri)));

	if (state->getValue<bool>("on"))
		data.insertValue(SensorValue(LED_LIGHT_ON_OFF_MODULE_ID, 1));
	else
		data.insertValue(SensorValue(LED_LIGHT_ON_OFF_MODULE_ID, 0));

	return data;
}

list<ModuleType> PhilipsHueDimmableBulb::moduleTypes() const
{
	return BULB_MODULE_TYPES;
}

string PhilipsHueDimmableBulb::name() const
{
	return PHILIPS_BULB_NAME;
}

bool PhilipsHueDimmableBulb::decodeOnOffValue(const double value) const
{
	return (bool)value;
}
