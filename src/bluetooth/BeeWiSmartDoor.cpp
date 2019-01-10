#include <Poco/Exception.h>
#include <Poco/Timestamp.h>

#include "bluetooth/BeeWiSmartDoor.h"

#define OPEN_CLOSE_MODULE_ID 0
#define BATTERY_MODULE_ID 1

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static list<ModuleType> SENSOR_MODULE_TYPES = {
	{ModuleType::Type::TYPE_OPEN_CLOSE},
	{ModuleType::Type::TYPE_BATTERY},
};

const string BeeWiSmartDoor::NAME = "BeeWi Smart Door";

BeeWiSmartDoor::BeeWiSmartDoor(
		const MACAddress& address,
		const Timespan& timeout,
		const RefreshTime& refresh,
		const HciInterface::Ptr hci):
	BeeWiDevice(address, timeout, refresh, NAME, SENSOR_MODULE_TYPES, hci)
{
}

BeeWiSmartDoor::BeeWiSmartDoor(
		const MACAddress& address,
		const Timespan& timeout,
		const RefreshTime& refresh,
		const HciInterface::Ptr hci,
		HciConnection::Ptr conn):
	BeeWiDevice(address, timeout, refresh, NAME, SENSOR_MODULE_TYPES, hci)
{
	initLocalTime(conn);
}

BeeWiSmartDoor::~BeeWiSmartDoor()
{
}

SensorData BeeWiSmartDoor::parseAdvertisingData(
		const vector<unsigned char>& data) const
{
	if (data.size() != 5)
		throw ProtocolException("expected 5 B, received " + to_string(data.size()) + " B");

	double openClose = data[2];
	double battery = data[4];

	return {
		m_deviceId,
		Timestamp{},
		{
			{OPEN_CLOSE_MODULE_ID, openClose},
			{BATTERY_MODULE_ID, battery},
		}
	};
}

bool BeeWiSmartDoor::match(const string& modelID)
{
	return modelID.find("BeeWi BSDOO") != string::npos;
}
