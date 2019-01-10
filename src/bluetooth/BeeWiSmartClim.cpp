#include <Poco/Exception.h>
#include <Poco/Timestamp.h>

#include "bluetooth/BeeWiSmartClim.h"
#include "bluetooth/HciConnection.h"

#define TEMPERATURE_MODULE_ID 0
#define HUMIDITY_MODULE_ID 1
#define BATTERY_MODULE_ID 2

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static list<ModuleType> SENSOR_MODULE_TYPES = {
	{ModuleType::Type::TYPE_TEMPERATURE, {ModuleType::Attribute::ATTR_INNER}},
	{ModuleType::Type::TYPE_HUMIDITY, {ModuleType::Attribute::ATTR_INNER}},
	{ModuleType::Type::TYPE_BATTERY},
};

const string BeeWiSmartClim::NAME = "BeeWi SmartClim";

BeeWiSmartClim::BeeWiSmartClim(
		const MACAddress& address,
		const Timespan& timeout,
		const HciInterface::Ptr hci):
	BeeWiDevice(address, timeout, NAME, SENSOR_MODULE_TYPES, hci)
{
}

BeeWiSmartClim::~BeeWiSmartClim()
{
}

SensorData BeeWiSmartClim::parseAdvertisingData(
		const vector<unsigned char>& data) const
{
	if (data.size() != 11)
		throw ProtocolException("expected 11 B, received " + to_string(data.size()) + " B");

	double temperature;
	if (data[3] == 255)
		temperature = (data[2] - data[3]) / 10.0;
	else
		temperature = (data[2] + (data[3] << 8)) / 10.0;

	double humidity = data[5];
	double battery = data[10];

	return {
		m_deviceId,
		Timestamp{},
		{
			{TEMPERATURE_MODULE_ID, temperature},
			{HUMIDITY_MODULE_ID, humidity},
			{BATTERY_MODULE_ID, battery},
		}
	};
}

bool BeeWiSmartClim::match(const string& modelID)
{
	return modelID.find("BeeWi BBW200") != string::npos;
}
