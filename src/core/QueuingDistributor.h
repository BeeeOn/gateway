#pragma once

#include <Poco/AtomicCounter.h>
#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/Timestamp.h>

#include "core/AbstractDistributor.h"
#include "core/ExporterQueue.h"
#include "loop/StoppableRunnable.h"
#include "model/SensorData.h"

namespace BeeeOn {

class QueuingDistributor : public AbstractDistributor, public StoppableRunnable {
public:
	QueuingDistributor();
	~QueuingDistributor();

	void registerExporter(Poco::SharedPtr<Exporter> exporter) override;
	void exportData(const SensorData &sensorData) override;

	void setQueueCapacity(int capacity);
	void setQueueTreshold(int treshold);
	void setQueueBatchSize(int batchSize);

	/**
	 * The "not working" ExporterQueue tries to export data when the deadTimeout
	 * has elapsed since queue's thershold of fails was exceeded.
	 */
	void setDeadTimeout(int seconds);

	/**
	 * When all ExporterQueues are broken or empty, exporting thread sleeps
	 * for the idleTimeout. New incoming data wakes the thread up.
	 */
	void setIdleTimeout(int seconds);

	void run() override;
	void stop() override;

protected:
	std::vector<ExporterQueue::Ptr> m_queues;
	Poco::Event m_newData;
	Poco::AtomicCounter m_stop;
	Poco::Timespan m_deadTimeout;
	Poco::Timespan m_idleTimeout;
	int m_queueCapacity;
	int m_batchSize;
	int m_treshold;
};

}
