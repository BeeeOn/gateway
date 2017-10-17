#include <Poco/NumberParser.h>
#include <Poco/StringTokenizer.h>

#include "jablotron/JablotronDevice.h"
#include "jablotron/JablotronDeviceAC88.h"
#include "jablotron/JablotronDeviceJA82SH.h"
#include "jablotron/JablotronDeviceJA83P.h"
#include "jablotron/JablotronDeviceOpenClose.h"
#include "jablotron/JablotronDeviceRC86K.h"
#include "jablotron/JablotronDeviceTP82N.h"
#include "jablotron/JablotronDeviceJA85ST.h"

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
		return new JablotronDeviceAC88(deviceID);

	if ((serialNumber >= 0x180000) && (serialNumber <= 0x1BFFFF))
		return new JablotronDeviceOpenClose(deviceID, "JA-81M");

	if ((serialNumber >= 0x7F0000) && (serialNumber <= 0x7FFFFF))
		return new JablotronDeviceJA82SH(deviceID);

	if ((serialNumber >= 0x1C0000) && (serialNumber <= 0x1DFFFF))
		return new JablotronDeviceOpenClose(deviceID, "JA-83M");

	if ((serialNumber >= 0x640000) && (serialNumber <= 0x65FFFF))
		return new JablotronDeviceJA83P(deviceID);

	if ((serialNumber >= 0x760000) && (serialNumber <= 0x76FFFF))
		return new JablotronDeviceJA85ST(deviceID);

	if ((serialNumber >= 0x800000) && (serialNumber <= 0x97FFFF))
		return new JablotronDeviceRC86K(deviceID);

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
