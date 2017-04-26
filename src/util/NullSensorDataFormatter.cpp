#include <Poco/Exception.h>
#include <Poco/SingletonHolder.h>

#include "NullSensorDataFormatter.h"
#include "model/SensorData.h"

using namespace BeeeOn;
using namespace Poco;

NullSensorDataFormatter::NullSensorDataFormatter()
{
}

NullSensorDataFormatter::~NullSensorDataFormatter()
{
}

SensorDataFormatter &NullSensorDataFormatter::instance()
{
	static SingletonHolder<NullSensorDataFormatter> singleton;
	return *singleton.get();
}

std::string NullSensorDataFormatter::format(const SensorData &)
{
	throw NotImplementedException(__func__);
}
