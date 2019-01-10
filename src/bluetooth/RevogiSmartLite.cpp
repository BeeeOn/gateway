#include <cmath>

#include <Poco/Exception.h>

#include "bluetooth/RevogiSmartLite.h"
#include "bluetooth/HciConnection.h"

#define ON_OFF_MODULE_ID 0
#define BRIGHTNESS_MODULE_ID 1
#define COLOR_TEMPERATURE_MODULE_ID 2
#define COLOR_MODULE_ID 3

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const unsigned char MIN_COLOR_TEMP = 0;
static const unsigned char MAX_COLOR_TEMP = 200;
static const uint32_t MIN_SUPPORTED_COLOR_TEMP = 2700;
static const uint32_t MAX_SUPPORTED_COLOR_TEMP = 6500;
static const uint32_t MIN_BEEEON_COLOR_TEMP = 1700;
static const uint32_t MAX_BEEEON_COLOR_TEMP = 27000;

static const list<ModuleType> LIGHT_MODULE_TYPES = {
	{ModuleType::Type::TYPE_ON_OFF, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_BRIGHTNESS, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_COLOR_TEMPERATURE, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
	{ModuleType::Type::TYPE_COLOR, {ModuleType::Attribute::ATTR_CONTROLLABLE}},
};

const string RevogiSmartLite::LIGHT_NAME = "Delite-1748";

RevogiSmartLite::RevogiSmartLite(
		const MACAddress& address,
		const Timespan& timeout,
		const RefreshTime& refresh,
		const HciInterface::Ptr hci):
	RevogiRGBLight(address, timeout, LIGHT_NAME, LIGHT_MODULE_TYPES, refresh, hci)
{
}

RevogiSmartLite::~RevogiSmartLite()
{
}

void RevogiSmartLite::requestModifyState(
		const ModuleID& moduleID,
		const double value)
{
	SynchronizedObject::ScopedLock guard(*this);

	HciConnection::Ptr conn = m_hci->connect(m_address, m_timeout);
	vector<unsigned char> actualSetting = conn->notifiedWrite(
		ACTUAL_VALUES_GATT, WRITE_VALUES_GATT, NOTIFY_DATA, m_timeout);

	if (actualSetting.size() != 18)
		throw ProtocolException("expected 18 B, received " + to_string(actualSetting.size()) + " B");

	bool rgbMode = !actualSetting[9];
	uint32_t rgb = retrieveRGB(actualSetting);
	unsigned char colorTemp = actualSetting[8];

	switch (moduleID.value()) {
	case ON_OFF_MODULE_ID:
		if (rgbMode)
			RevogiRGBLight::modifyStatus(value, conn);
		else
			RevogiSmartLite::modifyStatus(value, conn);

		break;
	case BRIGHTNESS_MODULE_ID:
		if (rgbMode)
			RevogiRGBLight::modifyBrightness(value, rgb, conn);
		else
			RevogiSmartLite::modifyBrightness(value, colorTemp, conn);

		break;
	case COLOR_TEMPERATURE_MODULE_ID:
		modifyColorTemperature(value, conn);
		break;
	case COLOR_MODULE_ID:
		modifyColor(value, conn);
		break;
	default:
		throw InvalidArgumentException("invalid module ID: " + to_string(moduleID.value()));
	}
}

void RevogiSmartLite::modifyStatus(const double value, const HciConnection::Ptr conn)
{
	unsigned char inVal = value == 0 ? 0xff : 0xfe;
	unsigned char checksum = 4 - (0xff - inVal);

	sendWriteRequest(conn, {0, 0, 0, inVal, 0, 1}, checksum);
}

void RevogiSmartLite::modifyBrightness(
		const double value,
		const unsigned char colorTemperature,
		const HciConnection::Ptr conn)
{
	unsigned char inVal = brightnessFromPercents(value);
	unsigned char checksum = inVal + colorTemperature - 305;

	sendWriteRequest(conn, {0xfe, 0xf0, 0xdc, inVal, colorTemperature, 1}, checksum);
}

void RevogiSmartLite::modifyColorTemperature(
		const double value,
		const HciConnection::Ptr conn)
{
	unsigned char inVal = colorTempFromKelvins(value);
	unsigned char checksum = 137 - (MAX_COLOR_TEMP - inVal);

	sendWriteRequest(conn, {0xfc, 0xfc, 0xfc, 0xc8, inVal, 0x01}, checksum);
}

unsigned char RevogiSmartLite::colorTempFromKelvins(const double temperature) const
{
	if (temperature < MIN_BEEEON_COLOR_TEMP || temperature > MAX_BEEEON_COLOR_TEMP)
		throw InvalidArgumentException("color temperature is out of range");

	if (temperature <= MIN_SUPPORTED_COLOR_TEMP) {
		return MIN_COLOR_TEMP;
	}
	else if (temperature >= MAX_SUPPORTED_COLOR_TEMP) {
		return MAX_COLOR_TEMP;
	}
	else {
		double percents = (temperature - MIN_SUPPORTED_COLOR_TEMP) /
			(MAX_SUPPORTED_COLOR_TEMP - MIN_SUPPORTED_COLOR_TEMP);
		return round((percents * MAX_COLOR_TEMP) + MIN_COLOR_TEMP);
	}
}

unsigned int RevogiSmartLite::colorTempToKelvins(const double value) const
{
	if (value < MIN_COLOR_TEMP || value > MAX_COLOR_TEMP)
		throw InvalidArgumentException("value is out of range");

	double percents = (value - MIN_COLOR_TEMP) / (MAX_COLOR_TEMP - MIN_COLOR_TEMP);
	return round((percents * (MAX_SUPPORTED_COLOR_TEMP - MIN_SUPPORTED_COLOR_TEMP)) +
		MIN_SUPPORTED_COLOR_TEMP);
}

SensorData RevogiSmartLite::parseValues(const std::vector<unsigned char>& values) const
{
	if (values.size() != 18)
		throw ProtocolException("expected 18 B, received " + to_string(values.size()) + " B");

	double onOff = values[7] > 0xc8 ? 0 : 1;
	double brightness = brightnessToPercents(values[7]);
	double colorTemp;
	uint32_t rgb;

	bool rgbMode = !values[9];
	if (rgbMode) {
		colorTemp = 0;
		rgb = retrieveRGB(values);
	}
	else {
		colorTemp = colorTempToKelvins(values[8]);
		rgb = 0;
	}

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
