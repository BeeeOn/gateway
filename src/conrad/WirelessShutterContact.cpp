#include <Poco/JSON/Object.h>

#include "conrad/WirelessShutterContact.h"
#include "util/JsonUtil.h"

#define OPEN_CLOSE_MODULE_ID 0
#define RSSI_MODULE_ID 1

using namespace BeeeOn;
using namespace Poco::JSON;
using namespace std;

static list<ModuleType> DEVICE_MODULE_TYPES = {
	{ModuleType::Type::TYPE_OPEN_CLOSE},
	{ModuleType::Type::TYPE_RSSI},
};

const string WirelessShutterContact::PRODUCT_NAME = "HM-Sec-SC-2";

WirelessShutterContact::WirelessShutterContact(
		const DeviceID& id,
		const RefreshTime &refresh):
	ConradDevice(id, refresh, PRODUCT_NAME, DEVICE_MODULE_TYPES)
{
}

WirelessShutterContact::~WirelessShutterContact()
{
}

/**
 * Message example:
 * {
 *     "channels" : {
 *         "Main" : "open"
 *     },
 *     "dev" : "HM_30B0BE",
 *     "event" : "message",
 *     "model" : "HM-SEC-SC-2",
 *     "raw" : "A0C44A64130B0BEF11034013FC8",
 *     "rssi" : -52,
 *     "serial" : "LEQ1101988",
 *     "type" : "threeStateSensor"
 * }
 */
SensorData WirelessShutterContact::parseMessage(const Object::Ptr message)
{
	SensorData data;
	data.setDeviceID(m_deviceId);

	Object::Ptr object = message->getObject("channels");

	if (object->getValue<std::string>("Main") == "open")
		data.insertValue(SensorValue(OPEN_CLOSE_MODULE_ID, 1));
	else
		data.insertValue(SensorValue(OPEN_CLOSE_MODULE_ID, 0));

	data.insertValue(SensorValue(RSSI_MODULE_ID, message->getValue<double>("rssi")));

	return data;
}
