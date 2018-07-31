#pragma once

#include "util/SensorDataParser.h"

namespace BeeeOn {

/**
 * @brief ChecksumSensorDataParser parses data serialized by the
 * equivalently configured ChecksumSensorDataFormatter. It first
 * extracts the checksum at the beginning of the given string.
 * Then the wrapped parser is used to parse the rest.
 */
class ChecksumSensorDataParser : public SensorDataParser {
public:
	ChecksumSensorDataParser();
	ChecksumSensorDataParser(SensorDataParser::Ptr parser);

	/**
	 * @brief Set delimiter between the prepended checksum and the
	 * actual data part formatted by the wrapped formatter.
	 */
	void setDelimiter(const std::string &delimiter);
	void setParser(SensorDataParser::Ptr parser);

	/**
	 * @brief Parse the given data string. Expect it to start
	 * with a checksum following by the given delimiter. The
	 * rest of the string is parsed by the wrapped parser.
	 */
	SensorData parse(const std::string &data) const override;

protected:
	/**
	 * @brief Parse the input data in case when the preset delimiter is empty.
	 */
	SensorData parseNoDelimiter(const std::string &data) const;

	/**
	 * @brief Check the given checksum and parse the content
	 * by the configured parser.
	 */
	SensorData checkAndParse(
		const std::string &prefix,
		const std::string &content) const;

private:
	std::string m_delimiter;
	SensorDataParser::Ptr m_parser;
};

}
