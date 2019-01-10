#include <Poco/DateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/Exception.h>

#include "bluetooth/BeeWiDevice.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const UUID BeeWiDevice::LOCAL_TIME = UUID("a8b3fd02-4834-4051-89d0-3de95cddd318");
const string BeeWiDevice::VENDOR_NAME = "BeeWi";

BeeWiDevice::BeeWiDevice(
		const MACAddress& address,
		const Timespan& timeout,
		const string& productName,
		const list<ModuleType>& moduleTypes,
		const HciInterface::Ptr hci):
	BLESmartDevice(address, timeout, hci),
	m_productName(productName),
	m_moduleTypes(moduleTypes),
	m_paired(false)
{
}

BeeWiDevice::~BeeWiDevice()
{
	if (m_paired.value() == true)
		m_hci->unwatch(m_address);
}


list<ModuleType> BeeWiDevice::moduleTypes() const
{
	return m_moduleTypes;
}

string BeeWiDevice::vendor() const
{
	return VENDOR_NAME;
}

string BeeWiDevice::productName() const
{
	return m_productName;
}

void BeeWiDevice::pair(
		Poco::SharedPtr<HciInterface::WatchCallback> callback)
{
	if (m_paired.value() == true)
		return;

	m_hci->watch(m_address, callback);
	m_paired = true;
}

void BeeWiDevice::initLocalTime(HciConnection::Ptr conn) const
{
	const DateTime localDate;
	const string strDate = DateTimeFormatter::format(localDate, "%y%m%d%H%M%S");

	try {
		conn->write(LOCAL_TIME, {strDate.begin(), strDate.end()});
	}
	catch (const IOException& e) {
		throw IllegalStateException("failed to init time on BeeWi device", e);
	}
}
