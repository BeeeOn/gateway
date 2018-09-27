#pragma once

#include <Poco/DOM/Node.h>

#include "util/TypeMappingParser.h"
#include "util/XmlTypeMappingParserHelper.h"

namespace BeeeOn {

/**
 * @brief XmlTypeMappingParser is an abstract specialization of the
 * TypeMappingParser. It is used to parse an external input stream
 * with type mapping definitions represented by a XML document.
 *
 * @see XmlTypeMappingParserHelper
 */
template <typename TechType>
class XmlTypeMappingParser : public TypeMappingParser<TechType>, protected Loggable {
public:
	XmlTypeMappingParser(
		const std::string &mappingGroup,
		const std::string &techNode,
		Poco::Logger &logger);

	/**
	 * @brief Parse the input stream as XML file via the XmlTypeMappingParserHelper.
	 *
	 * @see XmlTypeMappingParserHelper
	 */
	typename TypeMappingParser<TechType>::TypeMappingSequence parse(std::istream &in) override;

protected:
	/**
	 * @brief Parse the XML node describing a technology-specific data type.
	 * @throws Poco::SyntaxException
	 */
	virtual TechType parseTechType(const Poco::XML::Node &node) = 0;

	/**
	 * @returns string representation of the given technology-specific type.
	 */
	virtual std::string techTypeRepr(const TechType &type) = 0;

private:
	XmlTypeMappingParserHelper m_helper;
};

template <typename TechType>
XmlTypeMappingParser<TechType>::XmlTypeMappingParser(
		const std::string &mappingGroup,
		const std::string &techNode,
		Poco::Logger &logger):
	m_helper(mappingGroup, techNode, logger)
{
}

template <typename TechType>
typename TypeMappingParser<TechType>::TypeMappingSequence XmlTypeMappingParser<TechType>::parse(std::istream &in)
{
	typename TypeMappingParser<TechType>::TypeMappingSequence sequence;
	std::pair<Poco::AutoPtr<Poco::XML::Node>, ModuleType> mapping;

	if (logger().trace()) {
		logger().trace(
			"loading DOM representation of type mappings",
			__FILE__, __LINE__);
	}

	m_helper.parseDOM(in);

	while (!(mapping = m_helper.next()).first.isNull()) {
		const TechType techType = parseTechType(*mapping.first);

		if (logger().debug()) {
			logger().debug(
				"parsed mapping " + techTypeRepr(techType)
				+ " to " + mapping.second.type().toString(),
				__FILE__, __LINE__);
		}

		sequence.emplace_back(techType, mapping.second);
	}

	return sequence;
}

}
