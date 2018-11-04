#pragma once

namespace BeeeOn {

/**
 * @brief Report with data coming from a sensor. Each report
 * comes in format <code>[AAAAAAAA] TYPE PAYLOAD</code>.
 * The <code>AAAAAAAA</code> is an address in decadic format.
 * The <code>TYPE</code> represents type of the device.
 * The <code>PAYLOAD</code> contains the actual report that
 * depends on the <code>TYPE</code>.
 */
struct JablotronReport {
	/**
	 * @brief Address of the source device.
	 */
	const uint32_t address;

	/**
	 * @brief Type of device.
	 */
	const std::string type;

	/**
	 * @brief Data payload.
	 */
	const std::string data;

	/**
	 * @returns true if the report is valid.
	 */
	operator bool() const;

	/**
	 * @returns true if the report is invalid.
	 */
	bool operator !() const;

	/**
	 * @brief Search the payload for a keyword like <code>BEACON</code>,
	 * <code>SENSOR</code>, etc.
	 *
	 * If the argument hasValue is true then it is assumed that the
	 * keyword has a value separated by colon. In such case, data like
	 * <code>BEACON</code> are ignored due to a missing value. This is
	 * useful to search payload for e.g. <code>ACT:0</code>, <code>ACT:1</code>,
	 * etc.
	 */
	bool has(const std::string &keyword, bool hasValue = false) const;

	/**
	 * @returns value associated with the given keyword
	 * @throws Poco::NotFoundException is no such keyword with value
	 * is present in the payload
	 */
	int get(const std::string &keyword) const;

	/**
	 * @brief Almost same logic as for JablotronReport::get(), just
	 * the value is expected in the temperature format <code>##.#\xb0C</code>.
	 * @returns temperature value associated with the given keyword.
	 * @throws Poco::SyntaxException if value is not in a valid temperature format
	 * @throws Poco::NotFoundException is no such keyword with value
	 * is present in the payload
	 */
	double temperature(const std::string &keyword) const;

	/**
	 * @brief It calls <code>JablotronReport::get("LB")</code> internally
	 * and interprets the value as 1 - 5 % and 0 - 100 %.
	 * @returns battery status in percents
	 * @throws Poco::NotFoundException is no such keyword with value
	 * is present in the payload
	 */
	unsigned int battery() const;

	/**
	 * @returns string representation of the report
	 */
	std::string toString() const;

	/**
	 * @returns an invalid report
	 */
	static JablotronReport invalid();
};

}
