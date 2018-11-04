#pragma once

#include <map>
#include <string>
#include <vector>

#include <Poco/JSON/Object.h>

#include "model/DeviceID.h"
#include "model/SensorData.h"
#include "util/Loggable.h"

namespace BeeeOn {

class VPTValuesParser : protected Loggable {
public:
	static const std::map<std::string, int> BOILER_OPERATION_TYPE;
	static const std::map<std::string, int> BOILER_OPERATION_MODE;
	static const std::map<std::string, int> BOILER_STATUS;
	static const std::map<std::string, int> BOILER_MODE;

public:
	VPTValuesParser();
	~VPTValuesParser();

	std::vector<SensorData> parse(const DeviceID& id, const std::string& content);

protected:
	SensorData parseZone(const uint64_t zone, const DeviceID& id, const Poco::JSON::Object::Ptr json) const;
	SensorData parseBoiler(const DeviceID& id, const Poco::JSON::Object::Ptr json) const;
};

}
