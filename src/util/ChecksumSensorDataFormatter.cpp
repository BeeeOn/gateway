#include <Poco/Checksum.h>
#include <Poco/NumberFormatter.h>

#include "di/Injectable.h"
#include "util/ChecksumSensorDataFormatter.h"

BEEEON_OBJECT_BEGIN(BeeeOn, ChecksumSensorDataFormatter)
BEEEON_OBJECT_CASTABLE(SensorDataFormatter)
BEEEON_OBJECT_PROPERTY("delimiter", &ChecksumSensorDataFormatter::setDelimiter)
BEEEON_OBJECT_PROPERTY("formatter", &ChecksumSensorDataFormatter::setFormatter)
BEEEON_OBJECT_END(BeeeOn, ChecksumSensorDataFormatter)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

ChecksumSensorDataFormatter::ChecksumSensorDataFormatter():
	ChecksumSensorDataFormatter(nullptr)
{
}

ChecksumSensorDataFormatter::ChecksumSensorDataFormatter(SensorDataFormatter::Ptr formatter):
	m_delimiter("\t"),
	m_formatter(formatter)
{
}

void ChecksumSensorDataFormatter::setDelimiter(const string &delimiter)
{
	m_delimiter = delimiter;
}

void ChecksumSensorDataFormatter::setFormatter(SensorDataFormatter::Ptr formatter)
{
	m_formatter = formatter;
}

string ChecksumSensorDataFormatter::format(const SensorData &data)
{
	Checksum csum(Checksum::TYPE_CRC32);

	const auto &content = m_formatter->format(data);
	csum.update(content);

	return NumberFormatter::formatHex(csum.checksum(), 8)
		+ m_delimiter + content;
}
