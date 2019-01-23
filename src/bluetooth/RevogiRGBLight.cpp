#include <cmath>

#include <Poco/Exception.h>

#include "bluetooth/RevogiRGBLight.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const unsigned char MIN_BRIGHTNESS = 0;
static const unsigned char MAX_BRIGHTNESS = 200;
static const uint32_t MIN_COLOR = 1;
static const uint32_t MAX_COLOR = 16777215;

RevogiRGBLight::RevogiRGBLight(
		const MACAddress& address,
		const Timespan& timeout,
		const string& productName,
		const list<ModuleType>& moduleTypes,
		const RefreshTime& refresh,
		const HciInterface::Ptr hci):
	RevogiDevice(address, timeout, refresh, productName, moduleTypes, hci)
{
}

RevogiRGBLight::~RevogiRGBLight()
{
}

void RevogiRGBLight::modifyStatus(const double value, const HciConnection::Ptr conn)
{
	unsigned char inVal = value == 0 ? 0xff : 0xfe;
	unsigned char checksum = 3 - (0xff - inVal);

	sendWriteRequest(conn, {0, 0, 0, inVal, 0, 0}, checksum);
}

void RevogiRGBLight::modifyBrightness(
		const double value,
		const uint32_t rgb,
		const HciConnection::Ptr conn)
{
	unsigned char inVal = brightnessFromPercents(value);
	unsigned char checksum;
	unsigned char red = ((rgb >> 16) & 0xff);
	unsigned char green = ((rgb >> 8) & 0xff);
	unsigned char blue = rgb & 0xff;

	uint32_t tmpChecksum = colorChecksum(red, green, blue);
	checksum = static_cast<unsigned char>(tmpChecksum - MAX_BRIGHTNESS + inVal);

	sendWriteRequest(conn, {red, green, blue, inVal, 0, 0}, checksum);
}

void RevogiRGBLight::modifyColor(const double value, const HciConnection::Ptr conn)
{
	if (value < MIN_COLOR || value > MAX_COLOR)
		throw InvalidArgumentException("value is out of range");

	uint32_t rgb = value;
	uint8_t red = rgb >> 16;
	uint8_t green = rgb >> 8;
	uint8_t blue = rgb;

	uint32_t checksum = colorChecksum(red, green, blue);
	if (checksum > 0xff) {
		checksum--;
		sendWriteRequest(conn, {red, green, blue, 0xc7, 0, 0}, checksum);
	}
	else {
		sendWriteRequest(conn, {red, green, blue, 0xc8, 0, 0}, checksum);
	}
}

unsigned char RevogiRGBLight::brightnessFromPercents(const double percents) const
{
	if (percents < 0 || percents > 100)
		throw InvalidArgumentException("percents are out of range");

	return round((percents * (MAX_BRIGHTNESS - MIN_BRIGHTNESS)) / 100.0) + MIN_BRIGHTNESS;
}

unsigned int RevogiRGBLight::brightnessToPercents(const double value) const
{
	// the light is turned off
	if (value > MAX_BRIGHTNESS && value <= 0xff)
		return 0;

	if (value < MIN_BRIGHTNESS || value > MAX_BRIGHTNESS)
		throw InvalidArgumentException("value is out of range");

	return round(((value - MIN_BRIGHTNESS) / (MAX_BRIGHTNESS - MIN_BRIGHTNESS)) * 100);
}

uint32_t RevogiRGBLight::retrieveRGB(const std::vector<unsigned char>& values) const
{
	uint32_t rgb = values[4];
	rgb <<= 8;
	rgb |= values[5];
	rgb <<= 8;
	rgb |= values[6];

	return rgb;
}

uint32_t RevogiRGBLight::colorChecksum(
		const uint8_t red,
		const uint8_t green,
		const uint8_t blue) const
{
	uint32_t result;

	if (red >= green && red >= blue)
		result = -(0xff - red) + green + blue;
	else if (green >= red && green >= blue)
		result = -(0xff - green) + red + blue;
	else if (blue >= red && blue >= green)
		result = -(0xff - blue) + red + green;
	else
		poco_assert(false);

	return 0xcb + result;
}

void RevogiRGBLight::prependHeader(
		vector<unsigned char>& payload) const
{
	payload.insert(payload.begin(), {0x0f, 0x0d, 0x03, 0x00});
}

void RevogiRGBLight::appendFooter(
		vector<unsigned char>& payload,
		const unsigned char checksum) const
{
	payload.insert(payload.end(), {0x00, 0x00, 0x00, 0x00});
	RevogiDevice::appendFooter(payload, checksum);
}
