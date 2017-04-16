#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/NumberFormatter.h>
#include <Poco/Timespan.h>

#include "core/Distributor.h"
#include "core/VirtualSensor.h"
#include "di/Injectable.h"
#include "model/DeviceID.h"
#include "model/ModuleID.h"
#include "model/SensorData.h"
#include "model/SensorValue.h"
#include "util/ValueGenerator.h"

BEEEON_OBJECT_BEGIN(BeeeOn, VirtualSensor)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_REF("distributor", &VirtualSensor::setDistributor)
BEEEON_OBJECT_TEXT("deviceId", &VirtualSensor::setDeviceId)
BEEEON_OBJECT_NUMBER("refresh", &VirtualSensor::setRefresh)
BEEEON_OBJECT_NUMBER("min", &VirtualSensor::setMin)
BEEEON_OBJECT_NUMBER("max", &VirtualSensor::setMax)
BEEEON_OBJECT_END(BeeeOn, VirtualSensor)

using namespace Poco;
using namespace BeeeOn;

VirtualSensor::VirtualSensor():
	m_stopRequested(false),
	m_refresh(5),
	m_min(0),
	m_max(0)
{
}

VirtualSensor::~VirtualSensor()
{
}

void VirtualSensor::run()
{
	RandomGenerator randomGenerator;
	RangeGenerator generator(randomGenerator, m_min, m_max);

	while (!m_stopRequested) {
		if (!generator.hasNext()) {
			logger().warning("no more data to generate",
					__FILE__, __LINE__);
			break;
		}

		double value = generator.next();

		SensorData data;
		data.setDeviceID(m_deviceId);
		data.insertValue(SensorValue(ModuleID(0), value));

		m_distributor->exportData(data);
		logger().information("measured: "
				+ NumberFormatter::format(value, 6, 2),
				__FILE__, __LINE__);

		FastMutex::ScopedLock guard(m_lock);
		if (m_stopSignal.tryWait(m_lock, refreshMillis()))
			break;
	}
}

void VirtualSensor::stop()
{
	m_stopRequested = true;
	m_stopSignal.signal();
}

long VirtualSensor::refreshMillis() const
{
	return m_refresh * 1000;
}

void VirtualSensor::setRefresh(int secs)
{
	if (secs <= 0) {
		throw InvalidArgumentException(
			"refresh time must be a positive nuber");
	}

	m_refresh = secs;
}

void VirtualSensor::setMin(int min)
{
	m_min = min;
}

void VirtualSensor::setMax(int max)
{
	m_max = max;
}

void VirtualSensor::setDistributor(Poco::SharedPtr<Distributor> distributor)
{
	m_distributor = distributor;
}

void VirtualSensor::setDeviceId(const std::string &deviceId)
{
	m_deviceId = DeviceID::parse(deviceId);
}
