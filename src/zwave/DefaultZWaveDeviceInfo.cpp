#include "zwave/DefaultZWaveDeviceInfo.h"

using namespace BeeeOn;
using namespace std;

DefaultZWaveDeviceInfo::DefaultZWaveDeviceInfo()
{
	setRegistry(new ZWaveProductClassRegistry(ZWaveCommandClassMap()));
}

string DefaultZWaveDeviceInfo::convertValue(double value)
{
	return to_string(value);
}
