#include <Poco/Checksum.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>

#include "di/Injectable.h"
#include "util/ChecksumSensorDataParser.h"

BEEEON_OBJECT_BEGIN(BeeeOn, ChecksumSensorDataParser)
BEEEON_OBJECT_CASTABLE(SensorDataParser)
BEEEON_OBJECT_PROPERTY("delimiter", &ChecksumSensorDataParser::setDelimiter)
BEEEON_OBJECT_PROPERTY("parser", &ChecksumSensorDataParser::setParser)
BEEEON_OBJECT_END(BeeeOn, ChecksumSensorDataParser)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

ChecksumSensorDataParser::ChecksumSensorDataParser():
	ChecksumSensorDataParser(nullptr)
{
}

ChecksumSensorDataParser::ChecksumSensorDataParser(SensorDataParser::Ptr parser):
	m_delimiter("\t"),
	m_parser(parser)
{
}

void ChecksumSensorDataParser::setDelimiter(const string &delimiter)
{
	m_delimiter = delimiter;
}

void ChecksumSensorDataParser::setParser(SensorDataParser::Ptr parser)
{
	m_parser = parser;
}

SensorData ChecksumSensorDataParser::parse(const string &data) const
{
	const auto sep = data.find(m_delimiter);
	if (sep == string::npos)
		throw SyntaxException("missing checksum prefix");

	const auto &prefix = data.substr(0, sep);
	const auto &content = data.substr(sep + 1);

	return checkAndParse(prefix, content);
}

SensorData ChecksumSensorDataParser::checkAndParse(
		const string &prefix,
		const string &content) const
{
	const auto checksum = NumberParser::parseHex(prefix);

	Checksum csum(Checksum::TYPE_CRC32);
	csum.update(content);

	const auto &computed = csum.checksum();

	if (checksum != computed) {
		throw IllegalStateException(
			"checksum is invalid: "
			+ NumberFormatter::formatHex(checksum, 8)
			+ " != "
			+ NumberFormatter::formatHex(computed, 8));
	}

	return m_parser->parse(content);
}
