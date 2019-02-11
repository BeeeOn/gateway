#include <Poco/Exception.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/String.h>
#include <Poco/DOM/NamedNodeMap.h>

#include "iqrf/IQRFTypeMappingParser.h"

using namespace std;
using namespace Poco;
using namespace Poco::XML;
using namespace BeeeOn;

string IQRFType::toString() const
{
	string repr;
	repr += NumberFormatter::formatHex(id) + ", ";
	repr += NumberFormatter::formatHex(errorValue) + ", ";
	repr += NumberFormatter::format(wide) + ", ";
	repr += NumberFormatter::format(resolution) + ", ";
	repr += NumberFormatter::format(
		signedFlag,
		NumberFormatter::BoolFormat::FMT_YES_NO);

	return repr;
}

bool IQRFType::operator<(const IQRFType &type) const
{
	return type.id < id;
}

IQRFTypeMappingParser::IQRFTypeMappingParser(
		const string &mappingGroup,
		const string &techNode):
	XmlTypeMappingParser<IQRFType>(mappingGroup,
		techNode, Loggable::forClass(typeid(*this))),
	m_techNode(techNode)
{
}

IQRFType IQRFTypeMappingParser::parseTechType(const Node &node)
{
	const Node *idNode = node.attributes()->getNamedItem("id");
	if (idNode == nullptr)
		throw SyntaxException("missing attribute id on element " + m_techNode);

	const Node *errorValueNode = node.attributes()->getNamedItem("error-value");
	if (errorValueNode == nullptr)
		throw SyntaxException("missing attribute error-value on element " + m_techNode);

	const Node *wideNode = node.attributes()->getNamedItem("wide");
	if (wideNode == nullptr)
		throw SyntaxException("missing attribute wide on element " + m_techNode);

	const Node *resolutionNode = node.attributes()->getNamedItem("resolution");
	if (resolutionNode == nullptr)
		throw SyntaxException("missing attribute resolution on element " + m_techNode);

	const Node *signedFlagNode = node.attributes()->getNamedItem("signed");
	if (signedFlagNode == nullptr)
		throw SyntaxException("missing attribute signed on element " + m_techNode);

	const string id = trim(idNode->getNodeValue());
	const string errorValue = trim(errorValueNode->getNodeValue());
	const string wide = trim(wideNode->getNodeValue());
	const string resolution = trim(resolutionNode->getNodeValue());
	const string signedFlag = trim(signedFlagNode->getNodeValue());

	if (logger().trace()) {
		logger().trace(
			"parsed id:" + id
			+ " error-value: " + errorValue
			+ " wide: " + wide
			+ " resolution: " + resolution
			+ " signed: " + signedFlag,
			__FILE__, __LINE__);
	}

	return {
		NumberParser::parseHex(id),
		NumberParser::parseHex(errorValue),
		NumberParser::parseUnsigned(wide),
		NumberParser::parseFloat(resolution),
		NumberParser::parseBool(signedFlag)
	};
}

string IQRFTypeMappingParser::techTypeRepr(const IQRFType &type)
{
	return type.toString();
}
