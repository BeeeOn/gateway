#include <Poco/Exception.h>
#include <Poco/Timestamp.h>

#include "bluetooth/BeeWiSmartMotion.h"

#define MOTION_MODULE_ID 0
#define BATTERY_MODULE_ID 1

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static list<ModuleType> SENSOR_MODULE_TYPES = {
	{ModuleType::Type::TYPE_MOTION},
	{ModuleType::Type::TYPE_BATTERY},
};

const string BeeWiSmartMotion::NAME = "BeeWi Smart Motion";

BeeWiSmartMotion::BeeWiSmartMotion(const MACAddress& address, const Timespan& timeout):
	BeeWiDevice(address, timeout, NAME, SENSOR_MODULE_TYPES)
{
}

BeeWiSmartMotion::BeeWiSmartMotion(
		const MACAddress& address,
		const Timespan& timeout,
		HciConnection::Ptr conn):
	BeeWiDevice(address, timeout, NAME, SENSOR_MODULE_TYPES)
{
	initLocalTime(conn);
}

BeeWiSmartMotion::~BeeWiSmartMotion()
{
}

SensorData BeeWiSmartMotion::parseAdvertisingData(
		const vector<unsigned char>& data) const
{
	if (data.size() != 5)
		throw ProtocolException("expected 5 B, received " + to_string(data.size()) + " B");

	double motion = data[2];
	double battery = data[4];

	return {
		m_deviceId,
		Timestamp{},
		{
			{MOTION_MODULE_ID, motion},
			{BATTERY_MODULE_ID, battery},
		}
	};
}

bool BeeWiSmartMotion::match(const string& modelID)
{
	return modelID.find("BeeWi BSMOT") != string::npos;
}
