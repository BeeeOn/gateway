#include <list>

#include <Poco/NumberFormatter.h>

#include "vdev/VirtualDevice.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

VirtualDevice::VirtualDevice():
	m_refresh(5 * Timespan::SECONDS)
{
}

VirtualDevice::~VirtualDevice()
{
}

DeviceID VirtualDevice::deviceID() const
{
	return m_deviceID;
}

list<ModuleType> VirtualDevice::moduleTypes() const
{
	list<ModuleType> moduleTypeList;
	for (auto &item : m_modules)
		moduleTypeList.push_back(item->moduleType());

	return moduleTypeList;
}

void VirtualDevice::addModule(const VirtualModule::Ptr virtualModule)
{
	m_modules.push_back(virtualModule);
}

list<VirtualModule::Ptr> VirtualDevice::modules() const
{
	return m_modules;
}

SensorData VirtualDevice::generate()
{
	SensorData data;
	data.setDeviceID(deviceID());

	for (auto &item : m_modules) {
		if (item->generatorEnabled())
			data.insertValue(item->generate());
	}

	return data;
}

bool VirtualDevice::modifyValue(
	const ModuleID &moduleID, double value)
{
	for (auto &item : m_modules) {
		if (item->moduleID() == moduleID) {
			return item->modifyValue(value);
		}
	}

	return false;
}

Timespan VirtualDevice::refresh() const
{
	return m_refresh;
}

void VirtualDevice::setRefresh(Timespan refresh)
{
	if (refresh == 0)
		throw InvalidArgumentException(
			"invalid refresh: " + to_string(refresh.totalSeconds()));

	m_refresh = refresh;
}

void VirtualDevice::setDeviceId(const DeviceID &deviceId)
{
	m_deviceID = deviceId;
}

void VirtualDevice::setVendorName(const string &vendorName)
{
	m_vendorName = vendorName;
}

void VirtualDevice::setProductName(const string &productName)
{
	m_productName = productName;
}

string VirtualDevice::vendorName() const
{
	return m_vendorName;
}

string VirtualDevice::productName() const
{
	return m_productName;
}
