#include <Poco/Exception.h>

#include "core/QueuingDistributor.h"
#include "di/Injectable.h"

BEEEON_OBJECT_BEGIN(BeeeOn, QueuingDistributor)
BEEEON_OBJECT_CASTABLE(Distributor)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_REF("exporter", &QueuingDistributor::registerExporter)
BEEEON_OBJECT_NUMBER("deadTimeout", &QueuingDistributor::setDeadTimeout)
BEEEON_OBJECT_NUMBER("idleTimeout", &QueuingDistributor::setIdleTimeout)
BEEEON_OBJECT_NUMBER("queueCapacity", &QueuingDistributor::setQueueCapacity)
BEEEON_OBJECT_NUMBER("batchSize", &QueuingDistributor::setQueueBatchSize)
BEEEON_OBJECT_NUMBER("treshold", &QueuingDistributor::setQueueTreshold)
BEEEON_OBJECT_END(BeeeOn, QueuingDistributor)

using namespace BeeeOn;
using namespace Poco;
using namespace std;

const static Timespan DEFAULT_DEAD_TIMEOUT = Timespan(10 * Timespan::SECONDS);
const static Timespan DEFAULT_EMPTY_TIMEOUT = Timespan(5 * Timespan::SECONDS);
const static int DEFAULT_QUEUE_CAPACITY = 1000;
const static int DEFAULT_BATCH_SIZE = 30;
const static int DEFAULT_TRESHOLD = 10;

QueuingDistributor::QueuingDistributor():
	m_stop(false),
	m_deadTimeout(DEFAULT_DEAD_TIMEOUT),
	m_idleTimeout(DEFAULT_EMPTY_TIMEOUT),
	m_queueCapacity(DEFAULT_QUEUE_CAPACITY),
	m_batchSize(DEFAULT_BATCH_SIZE),
	m_treshold(DEFAULT_TRESHOLD)
{
}

QueuingDistributor::~QueuingDistributor()
{
}

void QueuingDistributor::setQueueCapacity(int capacity)
{
	if (capacity < 0)
		capacity = ExporterQueue::UNLIMITED_CAPACITY;

	m_queueCapacity = capacity;
}

void QueuingDistributor::setQueueTreshold(int treshold)
{
	if (treshold < 0)
		treshold = ExporterQueue::UNLIMITED_THRESHOLD;

	m_treshold = treshold;
}

void QueuingDistributor::setQueueBatchSize(int batchSize)
{
	if (batchSize < 0)
		batchSize = ExporterQueue::UNLIMITED_BATCH_SIZE;

	m_batchSize = batchSize;
}

void QueuingDistributor::setDeadTimeout(int seconds)
{
	if (seconds < 0)
		throw InvalidArgumentException("dead timeout must not be negative");

	m_deadTimeout = Timespan(seconds * Timespan::SECONDS);
}

void QueuingDistributor::setIdleTimeout(int seconds)
{
	if (seconds < 0)
		throw InvalidArgumentException("empty timeout must not be negative");

	m_idleTimeout = Timespan(seconds * Timespan::SECONDS);
}

void QueuingDistributor::registerExporter(SharedPtr<Exporter> exporter)
{
	ExporterQueue::Ptr queue = new ExporterQueue(exporter,
							m_batchSize,
							m_queueCapacity,
							m_treshold);

	logger().debug(string("exporter queue created:") +
		" batch size: " + to_string(m_batchSize) +
		"; capacity: " + to_string(m_queueCapacity) +
		"; treshold: " + to_string(m_treshold)
	);
	m_queues.push_back(queue);
}

void QueuingDistributor::run()
{
	logger().debug("distributor started");

	while (!m_stop) {
		unsigned int cannotExport = 0;

		for (auto q : m_queues) {
			if (q->canExport(m_deadTimeout)) {
				if (q->exportBatch() == 0)
					++cannotExport;
			}
			else {
				++cannotExport;
			}
		}

		// nothing was exported
		if (cannotExport == m_queues.size())
			m_newData.tryWait(m_idleTimeout.totalMilliseconds());
	}

	m_stop = false;
	logger().debug("distributor stopped");
}

void QueuingDistributor::stop()
{
	m_stop = true;

	// the event is set to prevent long waiting in run()
	m_newData.set();
}

void QueuingDistributor::exportData(const SensorData &sensorData)
{
	if (m_stop)
		return;

	for (auto q : m_queues)
		q->enqueue(sensorData);

	m_newData.set();
}
