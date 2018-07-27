#pragma once

#include <cstdint>

namespace BeeeOn {

/**
 * @brief The class stores color represent by red, green and blue component.
 * It allows to count the brightness according to the biggest RGB component.
 */
class ColorBrightness {
public:
	/**
	 * @param maxColorElement maximum allowed value of RGB component
	 */
	ColorBrightness(
		const uint8_t red,
		const uint8_t green,
		const uint8_t blue,
		const uint8_t maxColorElement = 0xff);

	/**
	 * @brief Returns brightness in percents.
	 */
	uint8_t brightness() const;

	/**
	 * @param brightness in percents
	 * @throw IllegalStateException in case of the brightness
	 * is bigger then 100
	 */
	void setBrightness(const uint8_t brightness);

	uint8_t red() const;
	uint8_t green() const;
	uint8_t blue() const;

	/**
	 * @throw IllegalStateException in case of some component
	 * is bigger than max color element
	 */
	void setColor(
		const uint8_t red,
		const uint8_t green,
		const uint8_t blue);

private:
	/**
	 * @throw IllegalStateException in case of some component
	 * is bigger than max color element
	 */
	void assertValidColor(
		const uint8_t red,
		const uint8_t green,
		const uint8_t blue) const;

	/**
	 * @brief Counts the brightness according to the biggest
	 * RGB component.
	 */
	void deriveBrightness(
		const uint8_t red,
		const uint8_t green,
		const uint8_t blue);

	/**
	 * @brief Normlizes the color which means that it adjusts
	 * each RGB component so that the brightness is 100%.
	 */
	void normalizeColor(
		const uint8_t red,
		const uint8_t green,
		const uint8_t blue);

	/**
	 * @brief Counts the brightness by the given RGB component.
	 * @return brightness
	 */
	uint8_t brightnessFromColor(const uint8_t component) const;

	/**
	 * @brief Omits the brightness from the given RGB component.
	 */
	uint8_t omitBrightness(const uint8_t component) const;

	/**
	 * @brief Applies the brightness to the given RGB component.
	 */
	uint8_t applyBrightness(const uint8_t component) const;

private:
	uint8_t m_red;
	uint8_t m_green;
	uint8_t m_blue;
	uint8_t m_brightness;

	uint8_t m_maxColorElement;
};

}
