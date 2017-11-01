#include <string>

#include <Poco/NumberFormatter.h>
#include <Poco/Timestamp.h>

#include "util/CSVSensorDataFormatter.h"
#include "di/Injectable.h"
#include "model/SensorData.h"

BEEEON_OBJECT_BEGIN(BeeeOn, CSVSensorDataFormatter)
BEEEON_OBJECT_CASTABLE(SensorDataFormatter)
BEEEON_OBJECT_TEXT("separator", &CSVSensorDataFormatter::setSeparator)
BEEEON_OBJECT_END(BeeeOn, CSVSensorDataFormatter)

#define DEFAULT_SEPARATOR ";"
#define PRECISION_OF_VALUE 2

using namespace BeeeOn;
using namespace Poco;
using namespace std;

CSVSensorDataFormatter::CSVSensorDataFormatter() :
	m_separator(DEFAULT_SEPARATOR)
{
}

string CSVSensorDataFormatter::format(const SensorData &data)
{
	string output = "";
	string device = data.deviceID().toString();
	string timestamp = to_string(data.timestamp().value().epochTime());

	for (auto item : data) {
		if (!output.empty())
			output += '\n';
		output += "sensor" + separator() + timestamp + separator() + device + separator();
		output += item.moduleID().toString() + separator();
		output += NumberFormatter::format(item.value(), PRECISION_OF_VALUE) + separator();
	}

	return output;
}
