#pragma once

#include <list>
#include <string>

#include <Poco/SharedPtr.h>

#include "model/ModuleID.h"
#include "model/ModuleType.h"
#include "model/SensorData.h"
#include "philips/PhilipsHueBridge.h"
#include "philips/PhilipsHueBulb.h"

namespace BeeeOn {

/**
 * @brief The class represents dimmable Philips Hue bulb.
 */
class PhilipsHueDimmableBulb : public PhilipsHueBulb {
public:
	typedef Poco::SharedPtr<PhilipsHueDimmableBulb> Ptr;

	PhilipsHueDimmableBulb(
		const uint32_t ordinalNumber,
		const PhilipsHueBridge::BulbID bulbId,
		const PhilipsHueBridge::Ptr bridge,
		const RefreshTime &refresh);

	bool requestModifyState(const ModuleID& moduleID, const double value) override;
	SensorData requestState() override;

	std::list<ModuleType> moduleTypes() const override;
	std::string name() const override;

private:
	bool decodeOnOffValue(const double value) const;
};

}
