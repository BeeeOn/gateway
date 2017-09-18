#include <Poco/Nullable.h>

#include "commands/NewDeviceCommand.h"

using namespace BeeeOn;
using namespace std;
using namespace Poco;

NewDeviceCommand::NewDeviceCommand(const DeviceID &deviceID, const string &vendor,
	const string &productName, const list<ModuleType> &dataTypes, Timespan refresh_time):
		m_deviceID(deviceID),
		m_vendor(vendor),
		m_productName(productName),
		m_dataTypes(dataTypes),
		m_refreshTime(refresh_time)
{
}

NewDeviceCommand::~NewDeviceCommand()
{
}

DeviceID NewDeviceCommand::deviceID() const
{
	return m_deviceID;
}

string NewDeviceCommand::vendor() const
{
	return m_vendor;
}

string NewDeviceCommand::productName() const
{
	return m_productName;
}

list<ModuleType> NewDeviceCommand::dataTypes() const
{
	return m_dataTypes;
}

bool NewDeviceCommand::supportRefreshTime() const
{
	return m_refreshTime.totalSeconds() < 0;
}

Timespan NewDeviceCommand::refreshtime() const
{
	return m_refreshTime;
}
