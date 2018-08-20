#pragma once

#include <iosfwd>

#include <Poco/AutoPtr.h>
#include <Poco/Logger.h>
#include <Poco/SharedPtr.h>

#include <Poco/DOM/Document.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeIterator.h>

#include "model/ModuleType.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief Helper defining method for parsing an input stream as
 * an XML input. The purpose is to extract information about type
 * mapping. The class is stateful and represents a single loop over
 * the parsed XML document.
 *
 * The helper expects the following XML structure:
 * <pre>
 *   <some-root>
 *     ...
 *     <some-mapping-group>
 *       <map comment="Type xxx">
 *         <TECH-SPEC TECH-ATTR="xxx" />
 *         <beeeon type="temperature,outer" />
 *       <map>
 *       <map comment="Type yyy">
 *         <TECH-SPEC TECH-ATTR="yyy" />
 *         <beeeon type="humidity" />
 *       </map>
 *       ...
 *     </some-mapping-group>
 *     ...
 *   </some-root>
 * </pre>
 *
 * The parser does not care about the depth of element <code><map></code>.
 * The <code><TECH-SPEC /></code> element is unknown to the helper
 * but it MUST be the previous sibling of the <code><beeeon /></code> element.
 * <code>TECH-ATTR</code> is technology-specific attribute that the parser
 * does not take care of either. The element <code><beeeon /></code> specifies
 * the BeeeOn type to map to.
 */
class XmlTypeMappingParserHelper : Loggable {
public:
	/**
	 * @param techNode - name of the technology-specific XML element
	 * @param logger - logger to log into
	 */
	XmlTypeMappingParserHelper(
		const std::string &techNode,
		Poco::Logger &logger);

	/**
	 * @brief Parse the given input stream and create its DOM representation
	 * internally.
	 */
	void parseDOM(std::istream &in);

	/**
	 * @returns the next available pair <code>(TECH-SPEC, ModuleType)</code>.
	 * The <code>TECH-SPEC</code> must be parsed by upper layer.
	 * @throws Poco::SyntaxException
	 */
	std::pair<Poco::AutoPtr<Poco::XML::Node>, ModuleType> next();

private:
	std::string m_techNode;
	Poco::AutoPtr<Poco::XML::Document> m_document;
	Poco::SharedPtr<Poco::XML::NodeIterator> m_iterator;
};

}

