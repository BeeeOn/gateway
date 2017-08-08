#include "bluetooth/BluetoothDevice.h"

using namespace BeeeOn;
using namespace std;

BluetoothDevice::BluetoothDevice(const DeviceID &id) :
	m_deviceID(id),
	m_status(UNKNOWN)
{
}

MACAddress BluetoothDevice::mac() const
{
	return MACAddress(m_deviceID.ident());
}

DeviceID BluetoothDevice::deviceID() const
{
	return m_deviceID;
}

BluetoothDevice::Status BluetoothDevice::status() const
{
	return m_status;
}

void BluetoothDevice::updateStatus(const Status &status)
{
	m_status = status;
}
