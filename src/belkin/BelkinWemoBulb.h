#pragma once

#include <list>
#include <string>

#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>

#include "belkin/BelkinWemoDevice.h"
#include "belkin/BelkinWemoLink.h"
#include "model/ModuleID.h"
#include "model/ModuleType.h"
#include "model/SensorData.h"

namespace BeeeOn {

/**
 * @brief The class represents Belkin WeMo Led Light Bulb. It works with
 * Belkin WeMo Link it uses his methods to turn off, turn on and
 * the brightness control of bulb.
 */
class BelkinWemoBulb : public BelkinWemoDevice {
public:
	typedef Poco::SharedPtr<BelkinWemoBulb> Ptr;

	/**
	 * @brief The DeviceID is created based on a bulb's 64-bit identifier,
	 * where DevicePrefix is given on 8th byte.
	 */
	BelkinWemoBulb(const BelkinWemoLink::BulbID bulbId, const BelkinWemoLink::Ptr link);
	~BelkinWemoBulb();

	bool requestModifyState(const ModuleID& moduleID, const double value) override;
	SensorData requestState() override;

	std::list<ModuleType> moduleTypes() const override;
	std::string name() const override;
	Poco::FastMutex& lock() override;

	BelkinWemoLink::Ptr link();

private:
	int dimToPercentage(const double value);
	int dimFromPercentage(const double percents);

private:
	BelkinWemoLink::BulbID m_bulbId;
	BelkinWemoLink::Ptr m_link;
};

}
