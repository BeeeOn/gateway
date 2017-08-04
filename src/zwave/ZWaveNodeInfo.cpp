#include <Poco/NumberParser.h>

#include <openzwave/Manager.h>

#include "zwave/ZWaveNodeInfo.h"

using namespace BeeeOn;
using namespace OpenZWave;
using namespace std;

using Poco::NumberParser;
using Poco::NotFoundException;

ZWaveNodeInfo::ZWaveNodeInfo():
	m_paired(false)
{
}

void ZWaveNodeInfo::setVendorName(const string &name)
{
	m_vendorName = name;
}

string ZWaveNodeInfo::vendorName() const
{
	return m_vendorName;
}

void ZWaveNodeInfo::setVendorID(uint32_t id)
{
	m_vendorID = id;
}
uint32_t ZWaveNodeInfo::vendorID() const
{
	return m_vendorID;
}

void ZWaveNodeInfo::setProductName(const string &name)
{
	m_productName = name;
}

string ZWaveNodeInfo::productName() const
{
	return m_productName;
}

void ZWaveNodeInfo::setProductID(uint32_t id)
{
	m_productID = id;
}

uint32_t ZWaveNodeInfo::productID() const
{
	return m_productID;
}

void ZWaveNodeInfo::setPolled(bool polled)
{
	m_polled = polled;
}

bool ZWaveNodeInfo::polled() const
{
	return m_polled;
}

void ZWaveNodeInfo::addValueID(
	const ValueID &valueID, const ModuleType &type)
{
	m_zwaveValues.push_back(
		pair<ValueID, ModuleType>(valueID, type)
	);
}

ZWaveNodeInfo ZWaveNodeInfo::build(uint32_t homeID, uint8_t nodeID)
{
	ZWaveNodeInfo nodeInfo;

	nodeInfo.setVendorName(
		Manager::Get()->GetNodeManufacturerName(homeID, nodeID)
	);

	nodeInfo.setVendorID(
		NumberParser::parseHex(Manager::Get()->GetNodeManufacturerId(homeID, nodeID))
	);

	nodeInfo.setProductName(
		Manager::Get()->GetNodeProductName(homeID, nodeID)
	);

	nodeInfo.setProductID(
		NumberParser::parseHex(Manager::Get()->GetNodeProductId(homeID, nodeID))
	);

	return nodeInfo;
}

vector<ZWaveNodeInfo::ZWaveValuePair> ZWaveNodeInfo::valueIDs() const
{
	return m_zwaveValues;
}

void ZWaveNodeInfo::setDeviceID(const DeviceID &deviceID)
{
	m_deviceID = deviceID;
}

DeviceID ZWaveNodeInfo::deviceID() const
{
	return m_deviceID;
}

ModuleID ZWaveNodeInfo::findModuleID(const ValueID &valueID) const
{
	for (unsigned int i = 0; i < m_zwaveValues.size(); ++i){
		if (m_zwaveValues.at(i).first == valueID)
			return ModuleID(i);
	}

	throw NotFoundException("ModuleID not found");
}

OpenZWave::ValueID ZWaveNodeInfo::findValueID(
	const unsigned int &index) const
{
	if (m_zwaveValues.size() <= index)
		throw NotFoundException("ValueID not found");

	return m_zwaveValues.at(index).first;
}

ModuleType ZWaveNodeInfo::findModuleType(const ValueID &valueID) const
{
	for (unsigned int i = 0; i < m_zwaveValues.size(); ++i){
		if (m_zwaveValues.at(i).first == valueID)
			return m_zwaveValues.at(i).second;
	}

	throw NotFoundException("ModuleType not found");
}

list<ModuleType> ZWaveNodeInfo::moduleTypes() const
{
	list<ModuleType> moduleTypes;
	for (auto &m_zwaveValue : m_zwaveValues)
		moduleTypes.push_back(m_zwaveValue.second);

	return moduleTypes;
}

void ZWaveNodeInfo::setPaired(bool paired)
{
	m_paired = paired;
}

bool ZWaveNodeInfo::paired() const
{
	return m_paired;
}
