#include <Poco/Exception.h>
#include <Poco/Timestamp.h>

#include "bluetooth/BeeWiSmartWatt.h"

#define ON_OFF_MODULE_ID 0
#define LIGHT_ON_OFF_MODULE_ID 1
#define POWER_MODULE_ID 2
#define VOLTAGE_MODULE_ID 3
#define CURRENT_MODULE_ID 4
#define FREQUENCY_MODULE_ID 5

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static list<ModuleType> DEVICE_MODULE_TYPES = {
	{ModuleType::Type::TYPE_ON_OFF, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_ON_OFF, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_POWER},
	{ModuleType::Type::TYPE_VOLTAGE},
	{ModuleType::Type::TYPE_CURRENT},
	{ModuleType::Type::TYPE_FREQUENCY},
};

const UUID BeeWiSmartWatt::ACTUAL_VALUES = UUID("a8b3ff07-4834-4051-89d0-3de95cddd318");
const UUID BeeWiSmartWatt::ON_OFF = UUID("a8b3ff04-4834-4051-89d0-3de95cddd318");
const UUID BeeWiSmartWatt::LIGHT_ON_OFF = UUID("a8b3ff06-4834-4051-89d0-3de95cddd318");
const string BeeWiSmartWatt::NAME = "BeeWi Smart Watt";

BeeWiSmartWatt::BeeWiSmartWatt(
		const MACAddress& address,
		const Timespan& timeout,
		const HciInterface::Ptr hci):
	BeeWiDevice(address, timeout, NAME, DEVICE_MODULE_TYPES, hci)
{
}

BeeWiSmartWatt::BeeWiSmartWatt(
		const MACAddress& address,
		const Timespan& timeout,
		const HciInterface::Ptr hci,
		HciConnection::Ptr conn):
	BeeWiDevice(address, timeout, NAME, DEVICE_MODULE_TYPES, hci)
{
	initLocalTime(conn);
}

BeeWiSmartWatt::~BeeWiSmartWatt()
{
}

void BeeWiSmartWatt::requestModifyState(
	const ModuleID& moduleID,
	const double value)
{
	SynchronizedObject::ScopedLock guard(*this);

	vector<unsigned char> data;
	UUID characteristics;

	switch (moduleID.value()) {
	case ON_OFF_MODULE_ID: {
		if (value != 0 && value != 1)
			throw IllegalStateException("value is not allowed");

		characteristics = ON_OFF;
		data.push_back(value);
		break;
	}
	case LIGHT_ON_OFF_MODULE_ID: {
		if (value != 0 && value != 1)
			throw IllegalStateException("value is not allowed");

		characteristics = LIGHT_ON_OFF;
		data.push_back(value);
		break;
	}
	default:
		throw IllegalStateException("invalid module ID: " + to_string(moduleID.value()));
	}

	HciConnection::Ptr conn = m_hci->connect(m_address, m_timeout);
	conn->write(characteristics, data);
}

SensorData BeeWiSmartWatt::requestState()
{
	SynchronizedObject::ScopedLock guard(*this);

	HciConnection::Ptr conn = m_hci->connect(m_address, m_timeout);
	vector<unsigned char> data = conn->read(ACTUAL_VALUES);

	return parseValues(data);
}

SensorData BeeWiSmartWatt::parseAdvertisingData(
		const vector<unsigned char>& data) const
{
	if (data.size() != 13)
		throw ProtocolException("expected 13 B, received " + to_string(data.size()) + " B");

	double onOff = data[2];
	double power = (data[6] + (data[7] << 8)) / 10.0;

	return {
		m_deviceId,
		Timestamp{},
		{
			{ON_OFF_MODULE_ID, onOff},
			{POWER_MODULE_ID, power},
		}
	};
}

SensorData BeeWiSmartWatt::parseValues(vector<unsigned char>& values) const
{
	if (values.size() != 7)
		throw ProtocolException("expected 7 B, received " + to_string(values.size()) + " B");

	double onOff = values[0];
	double power = (values[1] + (values[2] << 8)) / 10.0;
	double voltage = values[3];
	double current = (values[4] + (values[5] << 8)) / 1000.0;
	double frequency = values[6];

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

bool BeeWiSmartWatt::match(const string& modelID)
{
	return modelID.find("BeeWi BP1WC") != string::npos;
}
