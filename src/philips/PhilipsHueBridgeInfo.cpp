#include "philips/PhilipsHueBridgeInfo.h"
#include "util/JsonUtil.h"

using namespace BeeeOn;
using namespace Poco::JSON;
using namespace std;

PhilipsHueBridgeInfo::PhilipsHueBridgeInfo()
{
}

PhilipsHueBridgeInfo PhilipsHueBridgeInfo::buildBridgeInfo(
	const std::string& response)
{
	PhilipsHueBridgeInfo info;
	Object::Ptr object = JsonUtil::parse(response);

	info.m_name = object->getValue<string>("name");
	info.m_dataStoreVersion = object->getValue<string>("datastoreversion");
	info.m_swVersion = object->getValue<string>("swversion");
	info.m_apiVersion = object->getValue<string>("apiversion");
	info.m_mac = MACAddress::parse(object->getValue<string>("mac"));
	info.m_bridgeId = object->getValue<string>("bridgeid");
	info.m_factoryNew = object->getValue<bool>("factorynew");
	info.m_modelId = object->getValue<string>("modelid");

	return info;
}

string PhilipsHueBridgeInfo::name() const
{
	return m_name;
}

string PhilipsHueBridgeInfo::dataStoreVersion() const
{
	return m_dataStoreVersion;
}

string PhilipsHueBridgeInfo::swVersion() const
{
	return m_swVersion;
}

string PhilipsHueBridgeInfo::apiVersion() const
{
	return m_apiVersion;
}

MACAddress PhilipsHueBridgeInfo::mac() const
{
	return m_mac;
}

string PhilipsHueBridgeInfo::bridgeId() const
{
	return m_bridgeId;
}

bool PhilipsHueBridgeInfo::factoryNew() const
{
	return m_factoryNew;
}

string PhilipsHueBridgeInfo::modelId() const
{
	return m_modelId;
}
