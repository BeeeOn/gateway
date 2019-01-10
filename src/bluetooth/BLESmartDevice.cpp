#include <Poco/Exception.h>

#include "bluetooth/BLESmartDevice.h"
#include "model/DevicePrefix.h"

using namespace BeeeOn;
using namespace Poco;

BLESmartDevice::BLESmartDevice(
		const MACAddress& address,
		const Timespan& timeout,
		const HciInterface::Ptr hci):
	m_deviceId(DevicePrefix::PREFIX_BLE_SMART, address),
	m_address(address),
	m_timeout(timeout),
	m_hci(hci)
{
}

BLESmartDevice::~BLESmartDevice()
{
}

DeviceID BLESmartDevice::deviceID() const
{
	return m_deviceId;
}

MACAddress BLESmartDevice::macAddress() const
{
	return m_address;
}

void BLESmartDevice::pair(
		Poco::SharedPtr<HciInterface::WatchCallback>)
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
