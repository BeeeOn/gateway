#include <cmath>

#include <Poco/Logger.h>

#include "model/DevicePrefix.h"
#include "philips/PhilipsHueBulb.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const double PhilipsHueBulb::MAX_DIM = 255;

PhilipsHueBulb::PhilipsHueBulb(
		const uint32_t ordinalNumber,
		const PhilipsHueBridge::BulbID bulbId,
		const PhilipsHueBridge::Ptr bridge,
		const RefreshTime &refresh):
	m_deviceID(DevicePrefix::PREFIX_PHILIPS_HUE, bulbId & DeviceID::IDENT_MASK),
	m_ordinalNumber(ordinalNumber),
	m_bridge(bridge),
	m_refresh(refresh)
{
	m_bridge->incrementCountOfBulbs();
}

PhilipsHueBulb::~PhilipsHueBulb()
{
	try {
		m_bridge->decrementCountOfBulbs();
	}
	catch (IllegalStateException& e) {
		logger().log(e, __FILE__, __LINE__);
	}
}

DeviceID PhilipsHueBulb::deviceID()
{
	return m_deviceID;
}

DeviceID PhilipsHueBulb::id() const
{
	return m_deviceID;
}

RefreshTime PhilipsHueBulb::refresh() const
{
	return m_refresh;
}

FastMutex& PhilipsHueBulb::lock()
{
	return m_bridge->lock();
}

PhilipsHueBridge::Ptr PhilipsHueBulb::bridge()
{
	return m_bridge;
}

PhilipsHueBulbInfo PhilipsHueBulb::info()
{
	string response = m_bridge->requestDeviceState(m_ordinalNumber);
	return PhilipsHueBulbInfo::buildBulbInfo(response);
}

void PhilipsHueBulb::poll(Distributor::Ptr distributor)
{
	FastMutex::ScopedLock guard(lock());
	distributor->exportData(requestState());
}

int PhilipsHueBulb::dimToPercentage(const double value)
{
	if (value < 0 || value > MAX_DIM)
		throw InvalidArgumentException("value is out of range");

	return round((value / MAX_DIM) * 100.0);
}

int PhilipsHueBulb::dimFromPercentage(const double percents)
{
	if (percents < 0 || percents > 100)
		throw InvalidArgumentException("percents are out of range");

	return round((percents * MAX_DIM) / 100.0);
}
