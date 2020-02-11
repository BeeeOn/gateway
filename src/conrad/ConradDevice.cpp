#include "conrad/ConradDevice.h"
#include <Poco/NumberParser.h>

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const string ConradDevice::VENDOR_NAME = "Conrad";

ConradDevice::ConradDevice(
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

ConradDevice::~ConradDevice()
{
}

DeviceID ConradDevice::id() const
{
	return m_deviceId;
}

RefreshTime ConradDevice::refreshTime() const
{
	return m_refresh;
}

list<ModuleType> ConradDevice::moduleTypes() const
{
	return m_moduleTypes;
}

string ConradDevice::vendor() const
{
	return VENDOR_NAME;
}

string ConradDevice::productName() const
{
	return m_productName;
}

string ConradDevice::constructFHEMDeviceId(const DeviceID& id)
{
	// conrad ID is filled in the last 6 characters of Device ID
	string conradID = id.toString().substr(12, id.toString().size());
	// must be formated as upper case to be acceptable by unpair command
	for (auto & c: conradID) c = toupper(c);

	return "HM_" + conradID;
}

bool ConradDevice::isNumber(const string& s)
{
	try {
		NumberParser::parseFloat(s);
	}
	catch (SyntaxException& e) {
		return false;
	}
	return true;
}
