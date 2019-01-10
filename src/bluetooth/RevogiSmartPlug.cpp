#include <Poco/Exception.h>

#include "bluetooth/RevogiSmartPlug.h"
#include "bluetooth/HciConnection.h"

#define ON_OFF_MODULE_ID 0
#define POWER_MODULE_ID 1
#define VOLTAGE_MODULE_ID 2
#define CURRENT_MODULE_ID 3
#define FREQUENCY_MODULE_ID 4

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const list<ModuleType> PLUG_MODULE_TYPES = {
	{ModuleType::Type::TYPE_ON_OFF, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_POWER},
	{ModuleType::Type::TYPE_VOLTAGE},
	{ModuleType::Type::TYPE_CURRENT},
	{ModuleType::Type::TYPE_FREQUENCY},
};

const string RevogiSmartPlug::PLUG_NAME = "MeterPlug-F19F";

RevogiSmartPlug::RevogiSmartPlug(
		const MACAddress& address,
		const Timespan& timeout,
		const RefreshTime& refresh,
		const HciInterface::Ptr hci):
	RevogiDevice(address, timeout, refresh, PLUG_NAME, PLUG_MODULE_TYPES, hci)
{
}

RevogiSmartPlug::~RevogiSmartPlug()
{
}

void RevogiSmartPlug::requestModifyState(
		const ModuleID& moduleID,
		const double value)
{
	SynchronizedObject::ScopedLock guard(*this);

	if (moduleID.value() != ON_OFF_MODULE_ID)
		InvalidArgumentException("module " + to_string(moduleID.value()) + " is not controllable");

	unsigned char inVal = value == 0 ? 0x00 : 0x01;
	unsigned char checksum = inVal + 4;

	HciConnection::Ptr conn = m_hci->connect(m_address, m_timeout);
	sendWriteRequest(conn, {inVal, 0, 0}, checksum);
}

SensorData RevogiSmartPlug::parseValues(const vector<unsigned char>& values) const
{
	if (values.size() != 19)
		throw ProtocolException("expected 19 B, received " + to_string(values.size()) + " B");

	double onOff = values[4];
	double power = ((values[8] << 8) + values[9]) / 1000.0;
	double voltage = values[10];
	double current = ((values[11] << 8) + values[12]) / 1000.0;
	double frequency = values[13];

	return {
		m_deviceId,
		Timestamp{},
		{
			{ON_OFF_MODULE_ID, onOff},
			{POWER_MODULE_ID, power},
			{VOLTAGE_MODULE_ID, voltage},
			{CURRENT_MODULE_ID, current},
			{FREQUENCY_MODULE_ID, frequency},
		}
	};
}

void RevogiSmartPlug::prependHeader(
		vector<unsigned char>& payload) const
{
	payload.insert(payload.begin(), {0x0f, 0x06, 0x03, 0x00});
}
