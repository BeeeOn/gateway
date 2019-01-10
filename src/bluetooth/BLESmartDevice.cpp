#include <Poco/Exception.h>

#include "bluetooth/BLESmartDevice.h"
#include "model/DevicePrefix.h"

using namespace BeeeOn;
using namespace Poco;

BLESmartDevice::BLESmartDevice(
		const MACAddress& address,
		const Timespan& timeout,
		const RefreshTime& refresh,
		const HciInterface::Ptr hci):
	m_deviceId(DevicePrefix::PREFIX_BLE_SMART, address),
	m_address(address),
	m_timeout(timeout),
	m_refresh(refresh),
	m_hci(hci)
{
}

BLESmartDevice::~BLESmartDevice()
{
}

DeviceID BLESmartDevice::id() const
{
	return m_deviceId;
}

RefreshTime BLESmartDevice::refresh() const
{
	return m_refresh;
}

MACAddress BLESmartDevice::macAddress() const
{
	return m_address;
}

bool BLESmartDevice::pollable() const
{
	return false;
}

void BLESmartDevice::pair(
		Poco::SharedPtr<HciInterface::WatchCallback>)
{
}

void BLESmartDevice::poll(Distributor::Ptr)
{
}

void BLESmartDevice::requestModifyState(
		const ModuleID&,
		const double)
{
	throw NotImplementedException(__func__ );
}

SensorData BLESmartDevice::requestState()
{
	throw NotImplementedException(__func__ );
}

SensorData BLESmartDevice::parseAdvertisingData(
		const std::vector<unsigned char>&) const
{
	throw NotImplementedException(__func__ );
}
