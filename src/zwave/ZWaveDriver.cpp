#include <Poco/RegularExpression.h>
#include <Poco/String.h>

#include <openzwave/Manager.h>

#include "zwave/ZWaveDriver.h"

using namespace BeeeOn;
using namespace OpenZWave;
using namespace std;

ZWaveDriver::ZWaveDriver(const string &driverPath) :
	m_driver(driverPath)
{
}

void ZWaveDriver::setDriverPath(const string &driverPath)
{
	m_driver = driverPath;
}

string ZWaveDriver::driverPath()
{
	return m_driver;
}

Driver::ControllerInterface ZWaveDriver::detectInterface() const
{
	const Poco::RegularExpression re("usb");

	if (re.match(Poco::toLower(m_driver)))
		return Driver::ControllerInterface_Hid;
	else
		return Driver::ControllerInterface_Serial;
}

bool ZWaveDriver::registerItself()
{
	return Manager::Get()->AddDriver(m_driver, detectInterface());
}

bool ZWaveDriver::unregisterItself()
{
	return Manager::Get()->RemoveDriver(m_driver);
}
