#pragma once

#include <list>
#include <string>

#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>

#include "model/DeviceID.h"
#include "model/ModuleID.h"
#include "model/ModuleType.h"
#include "model/SensorData.h"
#include "philips/PhilipsHueBulbInfo.h"
#include "philips/PhilipsHueBridge.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief Abstract class representing generic Philips Hue bulb.
 */
class PhilipsHueBulb : protected Loggable {
public:
	typedef Poco::SharedPtr<PhilipsHueBulb> Ptr;

	static const double MAX_DIM;

	/**
	 * @brief The DeviceID is created based on a bulb's 64-bit identifier,
	 * where DevicePrefix is given on 8th byte.
	 */
	PhilipsHueBulb(
		const uint32_t ordinalNumber,
		const PhilipsHueBridge::BulbID bulbId,
		const PhilipsHueBridge::Ptr bridge);
	~PhilipsHueBulb();

	virtual bool requestModifyState(const ModuleID& moduleID, const double value) = 0;
	virtual SensorData requestState() = 0;

	DeviceID deviceID();
	virtual std::list<ModuleType> moduleTypes() const = 0;
	virtual std::string name() const = 0;
	Poco::FastMutex& lock();

	PhilipsHueBridge::Ptr bridge();
	PhilipsHueBulbInfo info();

protected:
	int dimToPercentage(const double value);
	int dimFromPercentage(const double percents);

protected:
	const DeviceID m_deviceID;
	uint32_t m_ordinalNumber;
	PhilipsHueBridge::Ptr m_bridge;
};

}
