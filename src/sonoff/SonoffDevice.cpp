#include "sonoff/SonoffDevice.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const string SonoffDevice::VENDOR_NAME = "Sonoff";

SonoffDevice::SonoffDevice(
		const DeviceID& id,
		const RefreshTime &refresh,
		const string& productName,
		const list<ModuleType>& moduleTypes):
	m_deviceId(id),
	m_refresh(refresh),
	m_productName(productName),
	m_moduleTypes(moduleTypes)
{
}

SonoffDevice::~SonoffDevice()
{
}

DeviceID SonoffDevice::id() const
{
	return m_deviceId;
}

RefreshTime SonoffDevice::refreshTime() const
{
	return m_refresh;
}

list<ModuleType> SonoffDevice::moduleTypes() const
{
	return m_moduleTypes;
}

string SonoffDevice::vendor() const
{
	return VENDOR_NAME;
}

string SonoffDevice::productName() const
{
	return m_productName;
}

Timestamp SonoffDevice::lastSeen() const
{
	return m_lastSeen;
}
