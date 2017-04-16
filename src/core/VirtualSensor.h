#ifndef BEEEON_VIRTUAL_SENSOR_H
#define BEEEON_VIRTUAL_SENSOR_H

#include <string>

#include <Poco/SharedPtr.h>
#include <Poco/Mutex.h>
#include <Poco/Condition.h>

#include "loop/StoppableRunnable.h"
#include "model/DeviceID.h"
#include "core/Distributor.h"
#include "util/Loggable.h"

namespace BeeeOn {

class VirtualSensor :
	public StoppableRunnable,
	public Loggable {
public:
	VirtualSensor();
	~VirtualSensor();

	void run() override;
	void stop() override;

	void setDeviceId(const std::string &deviceId);
	void setDistributor(Poco::SharedPtr<Distributor> distributor);
	void setMin(int min);
	void setMax(int max);
	void setRefresh(int sec);

protected:
	long refreshMillis() const;

private:
	Poco::SharedPtr<Distributor> m_distributor;
	Poco::FastMutex m_lock;
	Poco::Condition m_stopSignal;
	volatile bool m_stopRequested;
	unsigned int m_refresh;
	double m_min;
	double m_max;

	DeviceID m_deviceId;
};

}

#endif
