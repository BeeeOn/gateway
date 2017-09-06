#include "util/JsonUtil.h"
#include "util/JSONSensorDataParser.h"

using namespace BeeeOn;
using namespace Poco;
using namespace Poco::JSON;
using namespace std;

SensorData JSONSensorDataParser::parse(const string &data) const
{
	SensorData sensorData;
	Object::Ptr object = JsonUtil::parse(data);

	DeviceID id = DeviceID::parse(object->getValue<string>("device_id"));
	sensorData.setDeviceID(id);

	sensorData.setTimestamp(Timestamp(object->getValue<Timestamp::TimeVal>("timestamp")));

	Array::Ptr array = object->getArray("data");

	for (unsigned int i = 0; i < array->size(); ++i) {
		Object::Ptr tmpObject = array->getObject(i);

		ModuleID tmpID = ModuleID::parse(tmpObject->getValue<string>("module_id"));
		double tmpValue = tmpObject->optValue<double>("value", NAN);

		sensorData.insertValue(SensorValue(tmpID, tmpValue));
	}

	return sensorData;
}
