#include <Poco/Nullable.h>

#include "commands/NewDeviceCommand.h"

using namespace BeeeOn;
using namespace std;
using namespace Poco;

NewDeviceCommand::NewDeviceCommand(const DeviceID &deviceID, const string &vendor,
	const string &productName, const list<ModuleType> &dataTypes, Timespan refreshTime):
		m_description(deviceID, vendor, productName, dataTypes, refreshTime)
{
}

NewDeviceCommand::~NewDeviceCommand()
{
}

DeviceID NewDeviceCommand::deviceID() const
{
	return m_description.id();
}

string NewDeviceCommand::vendor() const
{
	return m_description.vendor();
}

string NewDeviceCommand::productName() const
{
	return m_description.productName();
}

list<ModuleType> NewDeviceCommand::dataTypes() const
{
	return m_description.dataTypes();
}

bool NewDeviceCommand::supportsRefreshTime() const
{
	return m_description.refreshTime() < 0;
}

Timespan NewDeviceCommand::refreshTime() const
{
	return m_description.refreshTime();
}

string NewDeviceCommand::toString() const
{
	return m_description.toString();
}

const DeviceDescription &NewDeviceCommand::description() const
{
	return m_description;
}
