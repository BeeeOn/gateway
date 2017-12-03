#ifndef BEEEON_LOGGING_COLLECTOR_H
#define BEEEON_LOGGING_COLLECTOR_H

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
	void onHciStats(const HciInfo &info) override;
	void onDispatch(const Command::Ptr cmd) override;

private:
	Poco::AtomicCounter m_seenData;
};

}

#endif
