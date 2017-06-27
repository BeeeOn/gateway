#pragma once

#include <queue>

#include <Poco/AtomicCounter.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>

#include "core/Exporter.h"
#include "model/SensorData.h"
#include "util/Loggable.h"

namespace BeeeOn {

class ExporterQueue : protected Loggable {
public:
	typedef Poco::SharedPtr<ExporterQueue> Ptr;

	const static int UNLIMITED_BATCH_SIZE = 0;
	const static int UNLIMITED_CAPACITY = 0;
	const static int UNLIMITED_THRESHOLD = -1;

	/**
	 * If batchSize <= 0 then size of batch is unlimited.
	 * If capacity <= 0 then data count is unlimited.
	 * If treshold < 0 then treshold is unlimited.
	 */
	ExporterQueue(
		Poco::SharedPtr<Exporter> exporter,
		int batchSize,
		int capacity,
		int treshold);

	~ExporterQueue();

	void enqueue(const SensorData &sensorData);
	unsigned int exportBatch();

	unsigned int sent() const;
	unsigned int dropped() const;

	/**
	 * The method canExport returns true if queue is not empty and at least one
	 * of following conditions is met:
	 *	- queue is working
	 *	- queue is dead too long
	 */
	bool canExport(const Poco::Timespan &deadTimeout) const;

	bool working() const;

private:
	/**
	* The method deadTooLong returns true if queue is working, or if
	* deadTimeout is elapsed since queue changed its status to "not working".
	*/
	bool deadTooLong(const Poco::Timespan deadTimeout) const;

	void fail();
	bool isEmpty() const;

	SensorData &front();
	void pop();

private:
	mutable Poco::FastMutex m_queueMutex;

	Poco::SharedPtr<Exporter> m_exporter;

	Poco::AtomicCounter m_dropped;
	Poco::AtomicCounter m_sent;
	int m_fails;
	int m_treshold;

	std::queue<SensorData> m_queue;
	unsigned int m_capacity;
	unsigned int m_batchSize;

	Poco::Timestamp m_timeOfFailure;
	Poco::AtomicCounter m_working;
};

}
