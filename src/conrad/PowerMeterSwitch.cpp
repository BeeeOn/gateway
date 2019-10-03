#include <Poco/JSON/Object.h>

#include "conrad/PowerMeterSwitch.h"
#include "util/JsonUtil.h"

#define FREQUENCY_MODULE_ID 0
#define CURRENT_MODULE_ID 1
#define POWER_MODULE_ID 2
#define VOLTAGE_MODULE_ID 3
#define ON_OFF_MODULE_ID 4
#define RSSI_MODULE_ID 5

using namespace BeeeOn;
using namespace Poco::JSON;
using namespace std;

static list<ModuleType> DEVICE_MODULE_TYPES = {
	{ModuleType::Type::TYPE_FREQUENCY},
	{ModuleType::Type::TYPE_CURRENT},
	{ModuleType::Type::TYPE_POWER},
	{ModuleType::Type::TYPE_VOLTAGE},
	{ModuleType::Type::TYPE_ON_OFF},
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

/**
 * Message example:
 * {
 *     "channels": {
 *         "Main": "CMDs_processing...",
 *         "Pwr": {
 *             "state": "0"
 *         },
 *         "SenF": {
 *             "state": "50.02"
 *         },
 *         "SenI": {
 *             "state": "0"
 *         },
 *         "SenPwr": {
 *             "state": "0"
 *         },
 *         "SenU": {
 *             "state": "234"
 *         },
 *         "Sw": {
 *             "state": "off"
 *         }
 *     },
 *     "dev": "38D649",
 *     "event": "message",
 *     "raw": "A1614A01038D649F1103402080022643006840085C88600",
 *     "rssi": -42.0,
 *     "type": "powerMeter"
 * }
 */
SensorData PowerMeterSwitch::parseMessage(const Object::Ptr message)
{

	SensorData data;
	data.setDeviceID(m_deviceId);

	Object::Ptr object = message->getObject("channels");
	Object::Ptr tmp;

	tmp = object->getObject("SenF");
	if (isNumber(tmp->getValue<std::string>("state"))) {
		data.insertValue(
			SensorValue(FREQUENCY_MODULE_ID, tmp->getValue<double>("state")));
	}

	tmp = object->getObject("SenI");
	if (isNumber(tmp->getValue<std::string>("state"))) {
		data.insertValue(
			SensorValue(CURRENT_MODULE_ID, tmp->getValue<double>("state")));
	}

	tmp = object->getObject("SenPwr");
	if (isNumber(tmp->getValue<std::string>("state"))) {
		data.insertValue(
			SensorValue(POWER_MODULE_ID, tmp->getValue<double>("state")));
	}

	tmp = object->getObject("SenU");
	if (isNumber(tmp->getValue<std::string>("state"))) {
		data.insertValue(
			SensorValue(VOLTAGE_MODULE_ID, tmp->getValue<double>("state")));
	}

	tmp = object->getObject("Sw");
	if (tmp->getValue<std::string>("state").compare("on"))
		data.insertValue(SensorValue(ON_OFF_MODULE_ID, 1));
	else
		data.insertValue(SensorValue(ON_OFF_MODULE_ID, 0));

	data.insertValue(
		SensorValue(RSSI_MODULE_ID, message->getValue<double>("rssi")));

	return data;
}
