#include <exception>

#include <Poco/Exception.h>

#include "core/ExporterQueue.h"

using namespace BeeeOn;
using namespace std;
using namespace Poco;

ExporterQueue::ExporterQueue(
		Poco::SharedPtr<Exporter> exporter,
		int batchSize,
		int capacity,
		int treshold):
	m_exporter(exporter),
	m_dropped(0),
	m_sent(0),
	m_failDetector(treshold),
	m_capacity(capacity),
	m_batchSize(batchSize)
{
}

ExporterQueue::~ExporterQueue()
{
}

void ExporterQueue::enqueue(const SensorData &sensorData)
{
	FastMutex::ScopedLock lock(m_queueMutex);

	if (m_queue.size() >= m_capacity && m_capacity > 0) {
		m_queue.pop();
		++m_dropped;
	}

	m_queue.push(sensorData);
}

unsigned int ExporterQueue::exportBatch()
{
	if (isEmpty())
		return 0;

	unsigned int i = 0;

	try {
		for (i = 0; (i < m_batchSize || m_batchSize <= 0) && !isEmpty(); ++i) {
			if (m_exporter->ship(front())) {
				++m_sent;
				pop();
			}
			else {
				break;
			}
		}
	}
	catch (const Exception &e) {
		m_failDetector.fail();
		logger().log(e, __FILE__, __LINE__);
		return i;
	}
	catch (exception &e) {
		m_failDetector.fail();
		poco_critical(logger(), e.what());
		return i;
	}
	catch (...) {
		m_failDetector.fail();
		poco_critical(logger(), "unknown error occured while shipping data");
		return i;
	}

	if (i > 0)
		m_failDetector.success();

	return i;
}


bool ExporterQueue::canExport(const Timespan &deadTimeout) const
{
	if (isEmpty())
		return false;

	return deadTooLong(deadTimeout);
}

bool ExporterQueue::working() const
{
	return !m_failDetector.isFailed();
}

bool ExporterQueue::deadTooLong(const Timespan deadTimeout) const
{
	return working() || m_failDetector.lastFailBefore(deadTimeout);
}

bool ExporterQueue::isEmpty() const
{
	FastMutex::ScopedLock lock(m_queueMutex);
	return m_queue.empty();
}

unsigned int ExporterQueue::dropped() const
{
	return m_dropped;
}

unsigned int ExporterQueue::sent() const
{
	return m_sent;
}

SensorData &ExporterQueue::front()
{
	FastMutex::ScopedLock lock(m_queueMutex);
	return m_queue.front();
}

void ExporterQueue::pop()
{
	FastMutex::ScopedLock lock(m_queueMutex);
	m_queue.pop();
}
