#pragma once

#include <string>

#include "util/XmlTypeMappingParser.h"

namespace BeeeOn {

/**
 * @brief Represents one IQRF type.
 */
struct IQRFType {
	/**
	 * @brief Sensor type identification in IQRF.
	 */
	const unsigned int id;

	/**
	 * @brief Value that specifies a sensor error.
	 */
	const unsigned int errorValue;

	/**
	 * @brief Byte size of value.
	 */
	const unsigned int wide;

	/**
	 * @brief The resolution indicates how near two neighboring measured
	 * values can be so that the sensor was able to distinguish them
	 */
	const double resolution;

	/**
	 * @brief Represents if value is signed or not.
	 */
	const bool signedFlag;

	std::string toString() const;

	bool operator <(const IQRFType &type) const;
};

/**
 * @brief IQRFTypeMappingParser can parse XML files defining mappings
 * between IQRF types and BeeeOn ModuleTypes.
 */
class IQRFTypeMappingParser : public XmlTypeMappingParser<IQRFType> {
public:
	IQRFTypeMappingParser(
		const std::string &mappingGroup, const std::string &techNode);

protected:
	/**
	 * @brief Parse the given DOM node and extract attributes:
	 *
	 *  - <code>id</code>
	 *  - <code>error-value</code>
	 *  - <code>wide</code>
	 *  - <code>resolution</code>
	 *  - <code>signed</code>
	 */
	IQRFType parseTechType(const Poco::XML::Node &node) override;

	std::string techTypeRepr(const IQRFType &type) override;

private:
	std::string m_techNode;
};

}
