#include <cmath>

#include <Poco/JSON/Object.h>

#include "sonoff/SonoffSC.h"
#include "util/JsonUtil.h"

#define TEMPERATURE_MODULE_ID 0
#define HUMIDITY_MODULE_ID 1
#define NOISE_MODULE_ID 2
#define DUST_MODULE_ID 3
#define LIGHT_MODULE_ID 4

using namespace BeeeOn;
using namespace Poco::JSON;
using namespace std;

static list<ModuleType> DEVICE_MODULE_TYPES = {
	{ModuleType::Type::TYPE_TEMPERATURE},
	{ModuleType::Type::TYPE_HUMIDITY},
	{ModuleType::Type::TYPE_NOISE},
	{ModuleType::Type::TYPE_PM25},
	{ModuleType::Type::TYPE_LUMINANCE},
};

const string SonoffSC::PRODUCT_NAME = "SC";

SonoffSC::SonoffSC(const DeviceID& id, const RefreshTime &refresh):
	SonoffDevice(id, refresh, PRODUCT_NAME, DEVICE_MODULE_TYPES)
{
}

SonoffSC::~SonoffSC()
{
}

/**
 * Example of MQTT message:
 * {
 *     temperature: 20,
 *     humidity: 50,
 *     noise: 30,
 *     dust: 2.35,
 *     light: 60
 * }
 */
SensorData SonoffSC::parseMessage(const MqttMessage& message)
{
	m_lastSeen.update();

	SensorData data;
	data.setDeviceID(m_deviceId);

	Object::Ptr object = JsonUtil::parse(message.message());

	if(object->has("temperature"))
		data.insertValue(SensorValue(TEMPERATURE_MODULE_ID, object->getValue<double>("temperature")));

	if(object->has("humidity"))
		data.insertValue(SensorValue(HUMIDITY_MODULE_ID, object->getValue<uint16_t>("humidity")));

	if(object->has("noise"))
		data.insertValue(SensorValue(NOISE_MODULE_ID, object->getValue<uint16_t>("noise")));

	if(object->has("dust"))
		data.insertValue(SensorValue(DUST_MODULE_ID, object->getValue<double>("dust")));

	if(object->has("light"))
		data.insertValue(SensorValue(LIGHT_MODULE_ID, object->getValue<uint16_t>("light")));

	return data;
}
