#include <list>

#include <Poco/Exception.h>
#include <Poco/NumberParser.h>
#include <Poco/Timespan.h>

#include <openzwave/Manager.h>

#include "model/ModuleType.h"
#include "zwave/ZWaveDeviceInfo.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

static const Timespan DEFAULT_REFRESH_TIME = -1;

ZWaveDeviceInfo::~ZWaveDeviceInfo()
{
}

Timespan ZWaveDeviceInfo::refreshTime() const
{
	return DEFAULT_REFRESH_TIME;
}

ZWaveClassRegistry::Ptr ZWaveDeviceInfo::registry() const
{
	return m_registry;
}

void ZWaveDeviceInfo::setRegistry(ZWaveClassRegistry::Ptr registry)
{
	m_registry = registry;
}

double ZWaveDeviceInfo::extractValue(
	const OpenZWave::ValueID &valueID, const ModuleType &)
{
	if (OpenZWave::ValueID::ValueType_Bool == valueID.GetType()) {
		bool boolValue;

		OpenZWave::Manager::Get()->GetValueAsBool(valueID, &boolValue);
		return boolValue;
	}

	string value;
	OpenZWave::Manager::Get()->GetValueAsString(valueID, &value);
	return NumberParser::parseFloat(value);
}

string ZWaveDeviceInfo::convertValue(double)
{
	throw NotImplementedException(__func__);
}
