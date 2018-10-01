#pragma once

#include <deque>

#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>

#include "core/Exporter.h"
#include "exporters/QueuingStrategy.h"
#include "model/SensorData.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
* @class QueuingExporter
* @brief Implements Exporter interface and provides SensorData prevents
*        SensorData loss.
*
* QueuingExporter serves as interface for any particular exporter. It prevents
* SensorData loss by buffering them and cooperating with a QueuingStrategy. By
* inheritance of this class, the particular exporter can work with the data via
* acquire(), ack() and reset() methods.
*
* As long as this class implements the Exporter interface, it can be used with
* any particular class which implements the Distributor interface. Despite
* the particular Distributor could provide its own data buffering, it is not
* needed for any particular Exporter that inherits from QueuingExporter.
* QueuingExporter always reports successful export and provides both
* the persistent and via the QueuingStrategy also non-persistent buffering.
*
* QueuingExporter has its own buffer, which serves as a temporary SensorData
* backup. When a situation that could lead to significant loss of the data occurs,
* or when the data loss is probable, the data from buffer are pushed
* to the QueuingStrategy.
*
* - The data loss is considered probable when the particular exporter does not
*   report successful export by calling the method ack() for the time longer
*   than the set threshold
*
* - The possible data loss is considered significant when the buffer contains
*   more data than the set threshold.
*/
class QueuingExporter : public Exporter, protected Loggable {
public:
	typedef Poco::SharedPtr<QueuingExporter> Ptr;

	QueuingExporter();
	~QueuingExporter() override;

	/**
	 * @param data SensorData to be enqueued
	 * @return always true
	 */
	bool ship(const SensorData &data) override;

	void setStrategy(const QueuingStrategy::Ptr strategy);

	/**
	 * When the number of enqueued SensorData is greater than or equal to
	 * saveThreshold, the data are pushed to the QueuingStrategy.
	 *
	 * If the push to the QueuingStrategy is not successful, the oldest data
	 * in the buffer, that are not acquired, are erased. After this erase,
	 * the amount of data in the buffer is equal to saveThreshold - 1.
	 *
	 * @param dataCount
	 */
	void setSaveThreshold(const int dataCount);

	/**
	 * When the given timeout is elapsed since the last successful export,
	 * enqueued data are pushed to the QueuingStrategy.
	 *
	 * @param timeout
	 */
	void setSaveTimeout(const Poco::Timespan timeout);

	/**
	 * Provided SensorData are mix from the queue and the QueuingStrategy.
	 * The strategyPriority gives the ratio of the provided data between
	 * the enqueued data and the data from the QueuingStrategy.
	 *
	 * @param percent
	 */
	void setStrategyPriority(const int percent);

protected:
	/**
	 * Acquires the data from the queue and from the QueuingStrategy.
	 *
	 * @param data Vector to be filled with the acquired data. This method does
	 * 	           not clear the vector, but just adds the data to the end of it.
	 * @param count The count of data to be acquired. This number gives
	 *              the maximum count, if there are less data available,
	 *              the vector is filled just by them (if no data are available,
	 *              the vector stays as it was given).
	 * @param timeout If there are no data available, this method waits for the
	 *                new data income, but for a maximum of this timeout.
	 */
	void acquire(
		std::vector<SensorData> &data,
		size_t count,
		const Poco::Timespan &timeout);

	/**
	 * When this method is called, all the previously acquired data are
	 * permanently deleted.
	 */
	void ack();

	/**
	 * After calling this method, no data are longer considered as acquired.
	 */
	void reset();

	/**
	 * @return True if the queue is empty.
	 */
	bool empty() const;

private:
	bool shouldSave() const;
	size_t queueSize() const;

	bool waitNotEmpty(const Poco::Timespan &timeout);

	/**
	 * This method serves to fill the vector by the appropriate SensorData from
	 * the buffer and the QueuingStrategy. If both the buffer and
	 * the QueuingStrategy are empty, given vector stays as given, else the data
	 * are added to the end of the vector.
	 *
	 * The count of the data to be required from the QueuingStrategy depends
	 * on the set backupPriority and the required data count. For example if
	 * the required data count is 10 and the backupPriority is 20%, 2 SensorData
	 * will be required from the QueuingStrategy.
	 *
	 * However, the result of this calculation could be a decimal number.
	 * In this case, the number is rounded down and the remainder is stored.
	 * The stored remainder is added to the result in the next call of this method.
	 * This eliminates the risk of unwanted suspension of the QueuingStrategy
	 * as the data source, as it is possible that the calculation result would
	 * always be less than 1.
	 *
	 * To avoid inconsistency in the case of calling this method multiple times
	 * without a successful export in between, two remainders are stored,
	 * one actual and one previous. If the successful export was not acknowledged
	 * since the last time this method was called the previous remainder is used
	 * in the calculation, to ensure that behaviour of the method will be equal
	 * to its last call.
	 *
	 * If the QueuingStrategy does not provide the required
	 * count of data, both remainders are reset to 0.
	 *
	 * The count of the data required from buffer is calculated as the difference
	 * of the required data count and the number of data loaded from strategy.
	 *
	 * A situation can also occur, when the sum of the SensorData in the buffer
	 * and the SensorData in the QueuingStrategy is greater than or equal to
	 * the required amount, but one of the sources has less data than is required
	 * from it based on its priority. In this case, all the data from that source
	 * are used to fill the vector, and the appropriate amount of data from another
	 * source is added, to provide the total required amount.
	 *
	 * @param data Vector to be filled
	 * @param count Required amount of SensorData
	 * @param acquired The count of data in the added to vector from the buffer
	 *                 is returned via this parameter
	 * @param peeked The count of data in the added to vector from
	 *               the QueuingStrategy is returned via this parameter
	 */
	void mix(
		std::vector<SensorData> &data,
		size_t count,
		size_t &acquired,
		size_t &peeked);

	size_t mixFromBackup(
		size_t requiredCount,
		size_t queueDataCount,
		size_t backupPriority,
		double remainder);

	void updateRemaindersAfterPeek(
			size_t peeked,
			size_t requiredToPeek,
			double potentialRemainder);

	size_t mixFromQueue(size_t toAcquire, size_t queueDataCount);

	void saveQueue(size_t skipFirst);
	void doSaveQueue(size_t skipFirst);

private:
	mutable Poco::Mutex m_queueMutex;

	QueuingStrategy::Ptr m_strategy;

	Poco::Event m_notEmpty;

	uint32_t m_backupPriority;
	size_t m_saveThreshold;
	Poco::Timespan m_saveTimeout;

	size_t m_acquiredDataCount;
	size_t m_peekedDataCount;
	Poco::Timestamp m_lastExport;
	std::deque<SensorData> m_queue;

	bool m_acked;
	double m_mixRemainder;
	double m_previousMixRemainder;
};

}
