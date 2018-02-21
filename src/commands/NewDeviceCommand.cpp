#include <Poco/Nullable.h>

#include "commands/NewDeviceCommand.h"

using namespace BeeeOn;
using namespace std;
using namespace Poco;

NewDeviceCommand::NewDeviceCommand(const DeviceID &deviceID, const string &vendor,
	const string &productName, const list<ModuleType> &dataTypes, Timespan refreshTime):
		m_deviceID(deviceID),
		m_vendor(vendor),
		m_productName(productName),
		m_dataTypes(dataTypes),
		m_refreshTime(refreshTime)
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

bool NewDeviceCommand::supportsRefreshTime() const
{
	return m_refreshTime < 0;
}

Timespan NewDeviceCommand::refreshTime() const
{
	return m_refreshTime;
}

string NewDeviceCommand::toString() const
{
	string cmdString;
	string modules;

	for (auto it = m_dataTypes.begin(); it != m_dataTypes.end(); ++it) {
		modules += it->type().toString();

		const auto &attributes = it->attributes();
		if (!attributes.empty())
			modules += ",";

		for (auto attr = attributes.begin(); attr != attributes.end(); ++attr) {
			modules += attr->toString();

			if (attr != --attributes.end())
				modules += ",";
		}

		if (it != --m_dataTypes.end())
			modules += " ";
	}

	cmdString += name() + " ";
	cmdString += m_deviceID.toString() + " ";
	cmdString += m_vendor + " ";
	cmdString += m_productName + " ";
	cmdString += to_string(m_refreshTime.totalSeconds()) + " ";
	cmdString += modules + " ";


	return cmdString;
}
