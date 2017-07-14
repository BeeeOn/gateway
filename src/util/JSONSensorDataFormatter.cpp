#include <iomanip>
#include <sstream>
#include <string>

#include <Poco/JSON/PrintHandler.h>
#include <Poco/NumberFormatter.h>
#include <Poco/Timestamp.h>

#include "di/Injectable.h"
#include "model/SensorData.h"
#include "util/JSONSensorDataFormatter.h"

BEEEON_OBJECT_BEGIN(BeeeOn, JSONSensorDataFormatter)
BEEEON_OBJECT_CASTABLE(SensorDataFormatter)
BEEEON_OBJECT_END(BeeeOn, JSONSensorDataFormatter)

using namespace BeeeOn;
using namespace Poco::JSON;
using namespace Poco;
using namespace std;

JSONSensorDataFormatter::JSONSensorDataFormatter()
{
}

string JSONSensorDataFormatter::format(const SensorData &data)
{
	stringstream output;
	PrintHandler json(output);

	json.startObject();

	json.key("device_id");
	json.value(data.deviceID().toString());
	json.key("timestamp");
	json.value((UInt64)data.timestamp().value().epochTime());

	json.key("data");
	json.startArray();

	for (auto item : data) {
		json.startObject();
		json.key("module_id");
		json.value(item.moduleID());

		if (item.isValid()) {
			json.key("value");
			if (std::isinf(item.value()) || std::isnan(item.value()))
				json.null();
			else
				json.value(item.value());
		}

		json.endObject();
	}

	json.endArray();
	json.endObject();
	return output.str();
}
