#include <cmath>

#include <Poco/Exception.h>
#include <Poco/Timestamp.h>

#include "bluetooth/BeeWiSmartLite.h"
#include "bluetooth/HciConnection.h"

#define ON_OFF_MODULE_ID 0
#define BRIGHTNESS_MODULE_ID 1
#define COLOR_TEMPERATURE_MODULE_ID 2
#define COLOR_MODULE_ID 3
#define MIN_BRIGHTNESS 2
#define MAX_BRIGHTNESS 11
#define MIN_COLOR_TEMP 2
#define MAX_COLOR_TEMP 11
#define MIN_SUPPORTED_COLOR_TEMP 3000
#define MAX_SUPPORTED_COLOR_TEMP 6000
#define MIN_COLOR 1
#define MAX_COLOR 16777215

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static list<ModuleType> LIGHT_MODULE_TYPES = {
	{ModuleType::Type::TYPE_ON_OFF, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_BRIGHTNESS, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_COLOR_TEMPERATURE, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_COLOR, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
};

const UUID BeeWiSmartLite::ACTUAL_VALUES = UUID("a8b3fff2-4834-4051-89d0-3de95cddd318");
const UUID BeeWiSmartLite::WRITE_VALUES = UUID("a8b3fff1-4834-4051-89d0-3de95cddd318");
const string BeeWiSmartLite::NAME = "BeeWi SmartLite";

BeeWiSmartLite::BeeWiSmartLite(
		const MACAddress& address,
		const Timespan& timeout,
		const HciInterface::Ptr hci):
	BeeWiDevice(address, timeout, NAME, LIGHT_MODULE_TYPES, hci)
{
}

BeeWiSmartLite::~BeeWiSmartLite()
{
}

void BeeWiSmartLite::requestModifyState(
	const ModuleID& moduleID,
	const double value)
{
	SynchronizedObject::ScopedLock guard(*this);

	vector<unsigned char> data;
	data.push_back(0x55);

	switch (moduleID.value()) {
	case ON_OFF_MODULE_ID: {
		if (value != 0 && value != 1)
			throw IllegalStateException("value is not allowed");

		data.push_back(Command::ON_OFF);
		data.push_back(value);
		break;
	}
	case BRIGHTNESS_MODULE_ID: {
		data.push_back(Command::BRIGHTNESS);
		data.push_back(brightnessFromPercentages(value));
		break;
	}
	case COLOR_TEMPERATURE_MODULE_ID: {
		data.push_back(Command::COLOR_TEMPERATURE);
		data.push_back(colorTempFromKelvins(value));
		break;
	}
	case COLOR_MODULE_ID: {
		if (value < MIN_COLOR || value > MAX_COLOR)
			throw IllegalStateException("value is out of range");

		uint32_t rgb = value;
		data.push_back(Command::COLOR);
		data.push_back(rgb >> 16);
		data.push_back(rgb >> 8);
		data.push_back(rgb);
		break;
	}
	default:
		throw IllegalStateException("invalid module ID: " + to_string(moduleID.value()));
	}

	data.push_back(0x0d);
	data.push_back(0x0a);

	HciConnection::Ptr conn = m_hci->connect(m_address, m_timeout);
	conn->write(WRITE_VALUES, data);
}

SensorData BeeWiSmartLite::parseAdvertisingData(
		const vector<unsigned char>& data) const
{
	if (data.size() != 8)
		throw ProtocolException("expected 8 B, received " + to_string(data.size()) + " B");

	double onOff = data[2];
	double brightness = brightnessToPercentages(data[4] >> 4);
	double colorTemp = colorTempToKelvins(data[4] & 0x0F);
	uint32_t rgb = data[5];
	rgb <<= 8;
	rgb |= data[6];
	rgb <<= 8;
	rgb |= data[7];

	return {
		m_deviceId,
		Timestamp{},
		{
			{ON_OFF_MODULE_ID, onOff},
			{BRIGHTNESS_MODULE_ID, brightness},
			{COLOR_TEMPERATURE_MODULE_ID, colorTemp},
			{COLOR_MODULE_ID, double(rgb)},
		}
	};
}

bool BeeWiSmartLite::match(const string& modelID)
{
	return modelID.find("BeeWi BLR") != string::npos;
}

unsigned int BeeWiSmartLite::brightnessToPercentages(const double value) const
{
	if (value < MIN_BRIGHTNESS || value > MAX_BRIGHTNESS)
		throw IllegalStateException("value is out of range");

	return round(((value - MIN_BRIGHTNESS) / (MAX_BRIGHTNESS - MIN_BRIGHTNESS)) * 100);
}

unsigned char BeeWiSmartLite::brightnessFromPercentages(const double percents) const
{
	if (percents < 0 || percents > 100)
		throw IllegalStateException("percents are out of range");

	return round((percents * (MAX_BRIGHTNESS - MIN_BRIGHTNESS)) / 100.0) + MIN_BRIGHTNESS;
}

unsigned int BeeWiSmartLite::colorTempToKelvins(const double value) const
{
	// The bulb is in mod RGB
	if (int(value) == 0)
		return 0;

	if (value < MIN_COLOR_TEMP || value > MAX_COLOR_TEMP)
		throw IllegalStateException("value is out of range");

	double percents = 1.0 - ((value - MIN_COLOR_TEMP) / (MAX_COLOR_TEMP - MIN_COLOR_TEMP));
	return round((percents * (MAX_SUPPORTED_COLOR_TEMP - MIN_SUPPORTED_COLOR_TEMP)) + MIN_SUPPORTED_COLOR_TEMP);
}

unsigned char BeeWiSmartLite::colorTempFromKelvins(const double temperature) const
{
	if (temperature < 1700 || temperature > 27000)
		throw IllegalStateException("color temperature is out of range");

	if (temperature <= MIN_SUPPORTED_COLOR_TEMP) {
		return MAX_COLOR_TEMP;
	}
	else if (temperature >= MAX_SUPPORTED_COLOR_TEMP) {
		return MIN_COLOR_TEMP;
	}
	else {
		double percents = 1.0 - (temperature - MIN_SUPPORTED_COLOR_TEMP) / (MAX_SUPPORTED_COLOR_TEMP - MIN_SUPPORTED_COLOR_TEMP);
		return round((percents * (MAX_COLOR_TEMP - MIN_COLOR_TEMP)) + MIN_COLOR_TEMP);
	}
}
