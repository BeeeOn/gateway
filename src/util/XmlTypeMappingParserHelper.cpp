#include <iostream>

#include <Poco/Exception.h>
#include <Poco/String.h>

#include <Poco/DOM/NamedNodeMap.h>
#include <Poco/DOM/NodeFilter.h>

#include "util/SecureXmlParser.h"
#include "util/XmlTypeMappingParserHelper.h"

using namespace std;
using namespace Poco;
using namespace Poco::XML;
using namespace BeeeOn;

XmlTypeMappingParserHelper::XmlTypeMappingParserHelper(
		const string &techNode,
		Logger &logger):
	m_techNode(techNode)
{
	setupLogger(&logger);
}

void XmlTypeMappingParserHelper::parseDOM(istream &in)
{
	SecureXmlParser parser;

	m_document = parser.parse(in);
	m_iterator = new NodeIterator(
			m_document->documentElement(),
			NodeFilter::SHOW_ELEMENT);
}

pair<AutoPtr<Node>, ModuleType> XmlTypeMappingParserHelper::next()
{
	Node *node = m_iterator->nextNode();

	while (node != nullptr) {
		if (node->localName() != "beeeon")
			continue;

		const Node *mapNode = node->parentNode();
		if (mapNode == nullptr) {
			logger().warning(
				"element beeeon has no parent",
				__FILE__, __LINE__);
		}
		else if (mapNode->localName() != "map") {
			logger().warning(
				"skipping element beeeon with parent '" + mapNode->localName() + "'",
				__FILE__, __LINE__);
			continue;
		}

		const Node *commentNode = mapNode->attributes()->getNamedItem("comment");
		if (commentNode == nullptr) {
			logger().warning(
				"missing comment attribute for element map",
				__FILE__, __LINE__);
		}
		else if (logger().debug()) {
			logger().debug(
				"parsing '" + commentNode->getNodeValue() + "'",
				__FILE__, __LINE__);
		}

		const Node *techNode = node->previousSibling();
		if (techNode->localName() != m_techNode) {
			if (logger().trace()) {
				logger().trace(
					"skipping element beeeon with previous sibling '" + techNode->localName() + "'",
					__FILE__, __LINE__);
			}

			continue;
		}

		const Node *typeNode = node->attributes()->getNamedItem("type");
		if (typeNode == nullptr)
			throw SyntaxException("missing attribute type on element beeeon");

		const string type = trim(typeNode->getNodeValue());
		return make_pair(techNode->cloneNode(false), ModuleType::parse(type));
	}

	return make_pair(nullptr, ModuleType{});
}
