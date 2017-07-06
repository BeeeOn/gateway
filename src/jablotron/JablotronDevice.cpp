#include <Poco/NumberParser.h>
#include <Poco/StringTokenizer.h>

#include "jablotron/JablotronDevice.h"
#include "jablotron/JablotronDeviceAC88.h"
#include "jablotron/JablotronDeviceTP82N.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const int LOW_BATTERY = 5;
static const int FULL_BATTERY = 100;

const Timespan JablotronDevice::REFRESH_TIME_SUPPORTED_BEACON = -1;
const Timespan JablotronDevice::REFRESH_TIME_UNSUPPORTED_BEACON = 9 * Timespan::MINUTES;

JablotronDevice::~JablotronDevice()
{
}

JablotronDevice::Ptr JablotronDevice::create(uint32_t serialNumber)
{
	DeviceID deviceID = JablotronDevice::buildID(serialNumber);

	if ((serialNumber >= 0xCF0000) && (serialNumber <= 0xCFFFFF))
		return new JablotronDeviceAC88(deviceID, "AC-88");

	if ((serialNumber >= 0x240000) && (serialNumber <= 0x25FFFF))
		return new JablotronDeviceTP82N(deviceID, "TP-82N");

	throw InvalidArgumentException(
		"unsupported device: " + to_string(serialNumber));
}

JablotronDevice::JablotronDevice(
		const DeviceID &deviceID, const string &name):
	m_deviceID(deviceID),
	m_name(name)
{
}

int JablotronDevice::parseValue(const string &msg) const
{
	StringTokenizer token(msg, ":");
	return NumberParser::parse(token[1]);
}

void JablotronDevice::setPaired(bool paired)
{
	m_paired = paired;
}

bool JablotronDevice::paired() const
{
	return m_paired;
}

DeviceID JablotronDevice::buildID(uint32_t serialNumber)
{
	return DeviceID(DevicePrefix::PREFIX_JABLOTRON, serialNumber);
}

DeviceID JablotronDevice::deviceID() const
{
	return m_deviceID;
}

int JablotronDevice::extractBatteryLevel(const string &battery)
{
	return parseValue(battery) ? LOW_BATTERY : FULL_BATTERY;
}

string JablotronDevice::name() const
{
	return m_name;
}

Timespan JablotronDevice::refreshTime()
{
	return REFRESH_TIME_UNSUPPORTED_BEACON;
}
