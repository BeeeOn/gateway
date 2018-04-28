#include "core/QueuingExporter.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;


QueuingExporter::QueuingExporter():
	m_notEmpty(false),
	m_backupPriority(20),
	m_saveThreshold(1000),
	m_saveTimeout(30 * Timespan::MINUTES),
	m_acquiredDataCount(0),
	m_peekedDataCount(0),
	m_acked(false),
	m_mixRemainder(0),
	m_previousMixRemainder(0)
{
}

QueuingExporter::~QueuingExporter()
{
	try {
		if (!empty())
			doSaveQueue(0);
	}
	BEEEON_CATCH_CHAIN(logger());
}

void QueuingExporter::setStrategy(const QueuingStrategy::Ptr &strategy)
{
	m_strategy = strategy;
}

void QueuingExporter::setSaveThreshold(int dataCount)
{
	if (dataCount <= 0)
		throw InvalidArgumentException("data threshold should be positive integer number");

	m_saveThreshold = dataCount;
}

void QueuingExporter:: setSaveTimeout(Timespan timeout)
{
	if (timeout < 0)
		throw InvalidArgumentException("save timeout should be positive");

	m_saveTimeout = timeout;
}

void QueuingExporter::setStrategyPriority(int percent)
{
	if (percent > 100 || percent < 0)
		throw InvalidArgumentException("backup priority should be in range within 0 and 100");

	m_backupPriority = (uint32_t) percent;
}

bool QueuingExporter::empty() const
{
	Mutex::ScopedLock lock(m_queueMutex);
	return m_queue.empty();
}

bool QueuingExporter::shouldSave() const
{
	Mutex::ScopedLock lock(m_queueMutex);
	return queueSize() >= m_saveThreshold
		   || m_lastExport.isElapsed(m_saveTimeout.totalMicroseconds());
}

size_t QueuingExporter::queueSize() const
{
	return m_queue.size();
}

void QueuingExporter::saveQueue(size_t skipFirst)
{
	Mutex::ScopedLock lock(m_queueMutex);

	if (!shouldSave())
		return;

	doSaveQueue(skipFirst);
}

void QueuingExporter::doSaveQueue(size_t skipFirst)
{
	Mutex::ScopedLock lock(m_queueMutex);
	vector<SensorData> tmp;

	const deque<SensorData>::iterator startFrom = next(m_queue.begin(), skipFirst);
	const deque<SensorData>::iterator oneBelowThreshold = next(startFrom, queueSize() - m_saveThreshold + 1);

	copy(startFrom, m_queue.end(), back_inserter(tmp));

	try {
		m_strategy->push(tmp);
		m_queue.erase(startFrom, m_queue.end());
	}
	BEEEON_CATCH_CHAIN_ACTION(logger(),
	    if (queueSize() > m_saveThreshold)
			m_queue.erase(oneBelowThreshold, m_queue.end());
	);
}

bool QueuingExporter::ship(const SensorData &data)
{
	Mutex::ScopedLock lock(m_queueMutex);

	m_queue.emplace_back(data);
	saveQueue(m_acquiredDataCount);

	if (!m_queue.empty())
		m_notEmpty.set();

	return true;
}

bool QueuingExporter::waitNotEmpty(const Timespan &timeout)
{
	return m_notEmpty.tryWait(timeout.totalMilliseconds());
}

void QueuingExporter::acquire(
		vector<SensorData> &data,
		size_t count,
		const Timespan &timeout)
{
	if (timeout < 0)
		throw InvalidArgumentException("timeout must be positive");

	if (m_queue.empty() && m_strategy->empty()) {
		if (!waitNotEmpty(timeout))
			return;
	}

	Mutex::ScopedLock lock(m_queueMutex);

	mix(data, count, m_acquiredDataCount, m_peekedDataCount);
	saveQueue(m_acquiredDataCount);
	m_acked = false;
}

void QueuingExporter::mix(vector<SensorData> &data, size_t count, size_t &acquired, size_t &peeked)
{
	Mutex::ScopedLock lock(m_queueMutex);

	if (m_backupPriority > 0 && !m_strategy->empty()) {

		// preserve recent remainder if not acked yet
		if (!m_acked)
			m_mixRemainder = m_previousMixRemainder;

		double realLoadCount = (count * m_backupPriority / 100.0) + m_mixRemainder;
		auto backupCount = mixFromBackup(count, queueSize(), m_backupPriority, m_mixRemainder);

		try {
			peeked = m_strategy->peek(data, backupCount);

			updateRemaindersAfterPeek(peeked, backupCount, realLoadCount - peeked);
		}
		BEEEON_CATCH_CHAIN(logger());
	}

	acquired = mixFromQueue(count - peeked, queueSize());

	copy(m_queue.begin(),
	     m_queue.begin() + acquired,
	     back_inserter(data)
	);
}

size_t QueuingExporter::mixFromBackup(
		size_t requiredCount,
		size_t queueDataCount,
		size_t backupPriority,
		double remainder)
{
	double realLoadCount = (requiredCount * backupPriority / 100.0) + remainder;
	auto backupCount = (size_t) realLoadCount;


	if (backupCount + queueDataCount < requiredCount)
		backupCount += requiredCount - (backupCount + queueDataCount);

	return backupCount;
}

void QueuingExporter::updateRemaindersAfterPeek(
		size_t peeked,
		size_t requiredToPeek,
		double potentialRemainder)
{
	if (peeked < requiredToPeek) {
		m_mixRemainder = 0;
		m_previousMixRemainder = 0;
		return;
	}

	m_previousMixRemainder = m_mixRemainder;
	m_mixRemainder = potentialRemainder;
}

size_t QueuingExporter::mixFromQueue(size_t toAcquire, size_t queueDataCount)
{
	if (toAcquire > queueDataCount)
		toAcquire = queueDataCount;

	return toAcquire;
}

void QueuingExporter::ack()
{
	Mutex::ScopedLock lock(m_queueMutex);

	m_queue.erase(m_queue.begin(), m_queue.begin() + m_acquiredDataCount);
	m_acquiredDataCount = 0;

	try {
		m_strategy->pop(m_peekedDataCount);
		m_peekedDataCount = 0;
	}
	BEEEON_CATCH_CHAIN(logger());

	m_lastExport.update();
	m_acked = true;
}

void QueuingExporter::reset()
{
	Mutex::ScopedLock lock(m_queueMutex);
	m_acquiredDataCount = 0;
	m_peekedDataCount = 0;
}
