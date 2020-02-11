#include <Poco/JSON/Object.h>
#include <Poco/Exception.h>

#include "conrad/PowerMeterSwitch.h"
#include "util/JsonUtil.h"

#define FREQUENCY_MODULE_ID 0
#define CURRENT_MODULE_ID 1
#define POWER_MODULE_ID 2
#define VOLTAGE_MODULE_ID 3
#define ON_OFF_MODULE_ID 4
#define RSSI_MODULE_ID 5

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::JSON;
using namespace std;

static list<ModuleType> DEVICE_MODULE_TYPES = {
	{ModuleType::Type::TYPE_FREQUENCY},
	{ModuleType::Type::TYPE_CURRENT},
	{ModuleType::Type::TYPE_POWER},
	{ModuleType::Type::TYPE_VOLTAGE},
	{ModuleType::Type::TYPE_ON_OFF, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_RSSI},
};

const string PowerMeterSwitch::PRODUCT_NAME = "HM-Es-PMSw1-PI";

PowerMeterSwitch::PowerMeterSwitch(
		const DeviceID& id,
		const RefreshTime &refresh):
	ConradDevice(id, refresh, PRODUCT_NAME, DEVICE_MODULE_TYPES)
{
}

PowerMeterSwitch::~PowerMeterSwitch()
{
}

void PowerMeterSwitch::requestModifyState(
		const ModuleID& moduleID,
		const double value,
		FHEMClient::Ptr fhemClient)
{
	if (moduleID.value() != ON_OFF_MODULE_ID) {
		throw InvalidArgumentException(
			"module " + to_string(moduleID.value()) + " is not controllable");
	}

	string fhemDeviceId = ConradDevice::constructFHEMDeviceId(m_deviceId);
	string request = "set " + fhemDeviceId + "_Sw ";
	if (value >= 1)
		request += "on";
	else
		request += "off";

	fhemClient->sendRequest(request);
}

/**
 * Message example:
 * {
 *    "channels" : {
 *        "Main" : "CMDs_done",
 *        "Pwr" : "32.6",
 *        "SenF" : "50.02",
 *        "SenI" : "0",
 *        "SenPwr" : "0",
 *        "SenU" : "239.4",
 *        "Sw" : "off"
 *    },
 *    "dev" : "HM_38D649",
 *    "event" : "message",
 *    "model" : "HM-ES-PMSW1-PL",
 *    "raw" : "A1478845E38D6490000008001460000000000095A02",
 *    "rssi" : -35.5,
 *    "serial" : "MEQ0106579",
 *    "type" : "powerMeter"
 * }
 */
SensorData PowerMeterSwitch::parseMessage(const Object::Ptr message)
{

	SensorData data;
	data.setDeviceID(m_deviceId);

	Object::Ptr object = message->getObject("channels");

	if (isNumber(object->getValue<std::string>("SenF"))) {
		data.insertValue(
			SensorValue(FREQUENCY_MODULE_ID, object->getValue<double>("SenF")));
	}

	if (isNumber(object->getValue<std::string>("SenI"))) {
		data.insertValue(
			SensorValue(CURRENT_MODULE_ID, object->getValue<double>("SenI")));
	}

	if (isNumber(object->getValue<std::string>("SenPwr"))) {
		data.insertValue(
			SensorValue(POWER_MODULE_ID, object->getValue<double>("SenPwr")));
	}

	if (isNumber(object->getValue<std::string>("SenU"))) {
		data.insertValue(
			SensorValue(VOLTAGE_MODULE_ID, object->getValue<double>("SenU")));
	}

	if (object->getValue<std::string>("Sw") == "on")
		data.insertValue(SensorValue(ON_OFF_MODULE_ID, 1));
	else
		data.insertValue(SensorValue(ON_OFF_MODULE_ID, 0));

	data.insertValue(
		SensorValue(RSSI_MODULE_ID, message->getValue<double>("rssi")));

	return data;
}
