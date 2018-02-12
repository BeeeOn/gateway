#include "philips/PhilipsHueBulbInfo.h"
#include "util/JsonUtil.h"

using namespace BeeeOn;
using namespace Poco::JSON;
using namespace std;

PhilipsHueBulbInfo::PhilipsHueBulbInfo()
{
}

PhilipsHueBulbInfo PhilipsHueBulbInfo::buildBulbInfo(
	const string& response)
{
	PhilipsHueBulbInfo info;
	Object::Ptr object = JsonUtil::parse(response);

	info.m_modules.emplace("on", to_string(object->getObject("state")->getValue<bool>("on")));
	info.m_modules.emplace("bri", to_string(object->getObject("state")->getValue<uint8_t>("bri")));

	info.m_reachable = object->getObject("state")->getValue<bool>("reachable");
	info.m_type = object->getValue<string>("type");
	info.m_name = object->getValue<string>("name");
	info.m_modelId = object->getValue<string>("modelid");
	info.m_manufacturerName = object->getValue<string>("manufacturername");
	info.m_uniqueId = object->getValue<string>("uniqueid");
	info.m_swVersion = object->getValue<string>("swversion");

	return info;
}

map<string, string> PhilipsHueBulbInfo::modules() const
{
	return m_modules;
}

bool PhilipsHueBulbInfo::reachable() const
{
	return m_reachable;
}

string PhilipsHueBulbInfo::type() const
{
	return m_type;
}

string PhilipsHueBulbInfo::name() const
{
	return m_name;
}

string PhilipsHueBulbInfo::modelId() const
{
	return m_modelId;
}

string PhilipsHueBulbInfo::manufacturerName() const
{
	return m_manufacturerName;
}

string PhilipsHueBulbInfo::uniqueId() const
{
	return m_uniqueId;
}

string PhilipsHueBulbInfo::swVersion() const
{
	return m_swVersion;
}
