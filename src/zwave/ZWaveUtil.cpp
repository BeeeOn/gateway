#include <command_classes/CommandClasses.h>

#include <Poco/NumberFormatter.h>

#include "zwave/ZWaveUtil.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

string ZWaveUtil::commandClass(const uint8_t cclass)
{
	return NumberFormatter::formatHex(cclass, 2)
		+ " "
		+ OpenZWave::CommandClasses::GetName(cclass);
}

string ZWaveUtil::commandClass(const uint8_t cclass, const uint8_t index)
{
	return NumberFormatter::formatHex(cclass, 2)
		+ ":"
		+ NumberFormatter::formatHex(index, 2)
		+ " "
		+ OpenZWave::CommandClasses::GetName(cclass);
}
