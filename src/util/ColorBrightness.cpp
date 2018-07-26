#include <cmath>
#include <string>

#include <Poco/Exception.h>

#include "util/ColorBrightness.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

ColorBrightness::ColorBrightness(
		const uint8_t red,
		const uint8_t green,
		const uint8_t blue,
		const uint8_t maxColorElement):
	m_maxColorElement(maxColorElement)
{
	setColor(red, green, blue);
}

uint8_t ColorBrightness::brightness() const
{
	return m_brightness;
}

void ColorBrightness::setBrightness(const uint8_t brightness)
{
	if (brightness > 100)
		throw IllegalStateException("brightness could not be greater than 100");

	m_brightness = brightness;
}

uint8_t ColorBrightness::red() const
{
	return applyBrightness(m_red);
}

uint8_t ColorBrightness::green() const
{
	return applyBrightness(m_green);
}

uint8_t ColorBrightness::blue() const
{
	return applyBrightness(m_blue);
}

void ColorBrightness::setColor(
		const uint8_t red,
		const uint8_t green,
		const uint8_t blue)
{
	assertValidColor(red, green, blue);

	normalizeColor(red, green, blue);
}

void ColorBrightness::assertValidColor(
		const uint8_t red,
		const uint8_t green,
		const uint8_t blue) const
{
	if (red > m_maxColorElement) {
		throw IllegalStateException(
			"red component(" + to_string(int(red)) + ") "
			"could not be bigger then " + to_string(int(m_maxColorElement)));
	}

	if (green > m_maxColorElement) {
		throw IllegalStateException(
			"green component(" + to_string(int(green)) + ") "
			"could not be bigger then " + to_string(int(m_maxColorElement)));
	}

	if (blue > m_maxColorElement) {
		throw IllegalStateException(
			"blue component(" + to_string(int(green)) + ") "
			"could not be bigger then " + to_string(int(m_maxColorElement)));
	}
}

void ColorBrightness::deriveBrightness(
		const uint8_t red,
		const uint8_t green,
		const uint8_t blue)
{
	if (red >= green && red >= blue)
		m_brightness = brightnessFromColor(red);
	else if (green >= red && green >= blue)
		m_brightness = brightnessFromColor(green);
	else
		m_brightness = brightnessFromColor(blue);
}

void ColorBrightness::normalizeColor(
		const uint8_t red,
		const uint8_t green,
		const uint8_t blue)
{
	deriveBrightness(red, green, blue);

	m_red = omitBrightness(red);
	m_green = omitBrightness(green);
	m_blue = omitBrightness(blue);
}

uint8_t ColorBrightness::brightnessFromColor(const uint8_t component) const
{
	return round((double(component) / m_maxColorElement) * 100);
}

uint8_t ColorBrightness::omitBrightness(const uint8_t component) const
{
	return round(component / (m_brightness / 100.0));
}

uint8_t ColorBrightness::applyBrightness(const uint8_t component) const
{
	return round(component * (m_brightness / 100.0));
}
