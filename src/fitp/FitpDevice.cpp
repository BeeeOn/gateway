#include <cmath>
#include <Poco/Logger.h>

#include "fitp/FitpDevice.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

#define BATTERY_PAYLOAD_SIZE      3
#define TEMPERATURE_PAYLOAD_SIZE  5
#define HUMIDITY_PAYLOAD_SIZE     5
#define RSSI_PAYLOAD_SIZE         1

#define SKIP_INFO                 5

#define UNAVAILABLE_MODULE        0x7f

#define U0   1800 //drained batteries [mV]

#define Umax 3200 //new batteries [mV]

static const uint8_t FITP_BATTERY_ID = 0;
static const uint8_t FITP_TEMPERATURE_INNER_ID = 1;
static const uint8_t FITP_TEMPERATURE_OUTER_ID = 2;
static const uint8_t FITP_HUMIDITY_ID = 3;
static const uint8_t FITP_ED_RSSI_ID = 4;
static const uint8_t FITP_COORD_RSSI_ID = 2;

static const uint8_t COORD_PREFIX = 0xec;

static const list<ModuleType> MODULES_COORD = {
	{ModuleType::Type::TYPE_BATTERY},
	{ModuleType::Type::TYPE_TEMPERATURE, {ModuleType::Attribute::ATTR_INNER}},
	{ModuleType::Type::TYPE_RSSI},
};

static const list<ModuleType> MODULES_ED = {
	{ModuleType::Type::TYPE_BATTERY},
	{ModuleType::Type::TYPE_TEMPERATURE, {ModuleType::Attribute::ATTR_INNER}},
	{ModuleType::Type::TYPE_TEMPERATURE, {ModuleType::Attribute::ATTR_OUTER}},
	{ModuleType::Type::TYPE_HUMIDITY, {ModuleType::Attribute::ATTR_INNER}},
	{ModuleType::Type::TYPE_RSSI},
};

FitpDevice::FitpDevice(DeviceID id):
	m_deviceID(id)
{
	if (((id & 0xFF000000) >> 24) == COORD_PREFIX)
		m_type = COORDINATOR;
	else
		m_type = END_DEVICE;
}

FitpDevice::~FitpDevice()
{
}

void FitpDevice::setDeviceID(const DeviceID &deviceId)
{
	m_deviceID = deviceId;
}

DeviceID FitpDevice::deviceID() const
{
	return m_deviceID;
}

FitpDevice::DeviceType FitpDevice::type() const
{
	return m_type;
}

list<ModuleType> FitpDevice::modules() const
{
	if (m_type == END_DEVICE)
		return MODULES_ED;
	else
		return MODULES_COORD;
}

size_t FitpDevice::moduleEDOffset(const uint8_t &id) const
{
	size_t count = 0;

	switch (id) {
	case FITP_BATTERY_ID:
		count = BATTERY_PAYLOAD_SIZE;
		break;
	case FITP_TEMPERATURE_INNER_ID:
	case FITP_TEMPERATURE_OUTER_ID:
		count = TEMPERATURE_PAYLOAD_SIZE;
		break;
	case FITP_HUMIDITY_ID:
		count = HUMIDITY_PAYLOAD_SIZE;
		break;
	case FITP_ED_RSSI_ID:
		count = RSSI_PAYLOAD_SIZE;
		break;
	default:
		logger().error(
			"module identifier is invalid",
			__FILE__, __LINE__
		);
		break;
	}

	return count;
}

size_t FitpDevice::moduleCOORDOffset(const uint8_t &id) const
{
	size_t count = 0;

	switch (id) {
	case FITP_BATTERY_ID:
		count = BATTERY_PAYLOAD_SIZE;
		break;
	case FITP_TEMPERATURE_INNER_ID:
		count = TEMPERATURE_PAYLOAD_SIZE;
		break;
	case FITP_COORD_RSSI_ID:
		count = RSSI_PAYLOAD_SIZE;
		break;
	default:
		logger().error(
			"module identifier is invalid",
			__FILE__, __LINE__
		);
		break;
	}

	return count;
}

double FitpDevice::voltsToPercentage(double milivolts)
{
	double percent;

	if (milivolts > (U0 + 10))
		percent = (milivolts - U0) / (Umax - U0) * 100;
	else
		percent = 1.0;

	if (percent > 100.0)
		percent = 100.0;

	return percent;
}

double FitpDevice::moduleValue(uint8_t id, const vector<uint8_t> &data)
{
	switch (id) {
	case FITP_BATTERY_ID:
		return voltsToPercentage(extractValue(data));
	case FITP_HUMIDITY_ID:
	case FITP_TEMPERATURE_INNER_ID:
	case FITP_TEMPERATURE_OUTER_ID:
		return extractValue(data) / 100.0;
	default:
		return NAN;
	}
}

double FitpDevice::extractValue(const vector<uint8_t> &values)
{
	if (values.empty())
		throw InvalidArgumentException("missing values of module in data message");

	if (values.at(0) == UNAVAILABLE_MODULE)
		throw InvalidArgumentException("unavailable module");

	int32_t data = 0;
	for (const uint8_t value : values) {
		data <<= 8;
		data |= value;
	}

	return double(data);
}

SensorData FitpDevice::parseMessage(const vector<uint8_t> &data, DeviceID deviceID) const
{

	SensorData sensorData;
	sensorData.setDeviceID(deviceID);

	for (auto it = data.begin() + SKIP_INFO; it != data.end();) {
		const uint8_t id = *it;

		ModuleID moduleID;
		size_t count;
		try {
			if (m_type == END_DEVICE) {
				moduleID = deriveEDModuleID(id);
				count = moduleEDOffset(id);
			}
			else {
				moduleID = deriveCOORDModuleID(id);
				count = moduleCOORDOffset(id);
			}
		}
		catch (const Exception &ex) {
			logger().log(ex, __FILE__, __LINE__);
			break;
		}

		if (count == 0)
			break;

		try {
			double value = moduleValue(id, {it + 1, it + count});
			// round the value to single decimal point
			value = round(value * 10.0) / 10.0;
			sensorData.insertValue(SensorValue(moduleID, value));
		}
		catch (const Exception &ex) {
			sensorData.insertValue(SensorValue(moduleID));
		}

		it += count;
	}

	return sensorData;
}

ModuleID FitpDevice::deriveEDModuleID(const uint8_t id)
{
	switch (id) {
	case FITP_BATTERY_ID:
		 return ModuleID(0);
	case FITP_TEMPERATURE_INNER_ID:
		return ModuleID(1);
	case FITP_TEMPERATURE_OUTER_ID:
		return ModuleID(2);
	case FITP_HUMIDITY_ID:
		return ModuleID(3);
	case FITP_ED_RSSI_ID:
		return ModuleID(4);
	default:
		throw InvalidArgumentException("invalid ED module: " + to_string(id));
	}
}

ModuleID FitpDevice::deriveCOORDModuleID(const uint8_t id)
{
	switch (id) {
	case FITP_BATTERY_ID:
		return ModuleID(0);
	case FITP_TEMPERATURE_INNER_ID:
		return ModuleID(1);
	case FITP_COORD_RSSI_ID:
		return ModuleID(2);
	default:
		throw InvalidArgumentException("invalid COORD module: " + to_string(id));
	}
}
