#pragma once

#include "util/SensorDataFormatter.h"

namespace BeeeOn {

/**
 * @brief ChecksumSensorDataFormatter is a wrapper around any SensorDataFormatter.
 * Its job is to utilize the wrapped formatter for the actual serialization and to
 * prepend a checksum at the beginning of the record.
 */
class ChecksumSensorDataFormatter : public SensorDataFormatter {
public:
	ChecksumSensorDataFormatter();
	ChecksumSensorDataFormatter(SensorDataFormatter::Ptr formatter);

	/**
	 * @brief Set delimiter between the prepended checksum and the
	 * actual data part formatted by the wrapped formatter.
	 */
	void setDelimiter(const std::string &delimiter);
	void setFormatter(SensorDataFormatter::Ptr formatter);

	/**
	 * @brief Format the given data via the configured formatter
	 * and prepend checksum of the resulted string. The result
	 * would be of the following form:
	 * <pre>
	 * CCCCCCCCD*S*
	 * </pre>
	 * where C represents a single checksum character, D* represents
	 * the delimiter string and S* the actual sensoric data string.
	 */
	std::string format(const SensorData &data) override;

private:
	std::string m_delimiter;
	SensorDataFormatter::Ptr m_formatter;
};

}
