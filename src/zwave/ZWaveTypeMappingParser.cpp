#include <Poco/Exception.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/String.h>

#include <Poco/DOM/NamedNodeMap.h>

#include "zwave/ZWaveTypeMappingParser.h"

using namespace std;
using namespace Poco;
using namespace Poco::XML;
using namespace BeeeOn;

ZWaveTypeMappingParser::ZWaveTypeMappingParser():
	XmlTypeMappingParser<ZWaveType>(
		"z-wave-mapping", "z-wave", Loggable::forClass(typeid(*this)))
{
}

ZWaveType ZWaveTypeMappingParser::parseTechType(const Node &node)
{
	const Node *ccNode = node.attributes()->getNamedItem("command-class");
	if (ccNode == nullptr)
		throw SyntaxException("missing attribute command-class on element z-wave");

	const Node *indexNode = node.attributes()->getNamedItem("index");

	const string cc = trim(ccNode->getNodeValue());
	const string index = indexNode == nullptr ? "0" : trim(indexNode->getNodeValue());

	if (logger().trace()) {
		logger().trace(
			"parsed " + cc + ":" + index,
			__FILE__, __LINE__);
	}

	return make_pair(NumberParser::parse(cc), NumberParser::parse(index));
}

string ZWaveTypeMappingParser::techTypeRepr(const ZWaveType &type)
{
	return NumberFormatter::format(type.first)
		+ ":" + NumberFormatter::format(type.second);
}
