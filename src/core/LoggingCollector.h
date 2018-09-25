#pragma once

#include <Poco/AtomicCounter.h>

#include "core/AbstractCollector.h"
#include "util/Loggable.h"

namespace BeeeOn {

class LoggingCollector : public AbstractCollector, Loggable {
public:
	LoggingCollector();
	~LoggingCollector();

	void onExport(const SensorData &data) override;
	void onDriverStats(const ZWaveDriverEvent &event) override;
	void onNodeStats(const ZWaveNodeEvent &event) override;
	void onNotification(const OZWNotificationEvent &event) override;
	void onHciStats(const HciInfo &info) override;
	void onBulbStats(const PhilipsHueBulbInfo &info) override;
	void onBridgeStats(const PhilipsHueBridgeInfo &info) override;
	void onDispatch(const Command::Ptr cmd) override;

private:
	Poco::AtomicCounter m_seenData;
};

}
