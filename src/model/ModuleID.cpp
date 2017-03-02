#include <climits>

#include <Poco/NumberParser.h>
#include <Poco/NumberFormatter.h>

#include "model/ModuleID.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

const uint16_t DEFAULT_MODULE_ID = 0;

ModuleID::ModuleID():
	m_moduleID(DEFAULT_MODULE_ID)
{
}

ModuleID::ModuleID(const uint16_t &moduleID):
	m_moduleID(moduleID)
{
}

ModuleID ModuleID::parse(const string &s)
{
	unsigned int moduleID;

	moduleID = Poco::NumberParser::parseUnsigned(s);
	if (moduleID > USHRT_MAX)
		throw RangeException(
			"module ID too high: " + to_string(moduleID));

	return ModuleID(moduleID);
}

string ModuleID::toString() const
{
	return Poco::NumberFormatter::formatHex(m_moduleID);
}

