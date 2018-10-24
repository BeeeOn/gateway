#pragma once

#include <queue>

#include <Poco/Event.h>
#include <Poco/Mutex.h>

#include "gwmessage/GWMessage.h"
#include "server/GWSConnector.h"
#include "server/GWSPriorityAssigner.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief Most GWSConnector implementations would solve the issue
 * of sending prioritization and asynchronous queuing of outgoing
 * messages. The AbstractGWSConnector aims on this issue.
 *
 * AbstractGWSConnector implements a queuing mechanism for outgoing messages.
 * Each message is appended to a queue based on its priority. The queue
 * number 0 is the most urgent queue.
 *
 * Each queue contains a statistic of count of messages sent from
 * the queue. A message for output is selected based on the following
 * algorithm:
 *
 * 1. take queue[i] (initially i := 0) with status[i]
 * 2. sum all status[j] for j > i
 * 3. if the status[i] <= sum(status[all j]), use queue i
 * 4. i := i + 1, try again
 *
 * Empty or unused queues are skipped. Summarization... The first queue must
 * always send more messages then all the other queues. If the first queue
 * sends more messages then available in other queues (the first queue has been
 * satisfied), the following queue is used with the same algorithm.
 */
class AbstractGWSConnector :
	public GWSConnector,
	protected Loggable {
public:
	AbstractGWSConnector();

	void setOutputsCount(int count);
	void setPriorityAssigner(GWSPriorityAssigner::Ptr assigner);

	/**
	 * @brief Setup queues based on configuration. This must
	 * be called before the connector is started.
	 */
	void setupQueues();

	/**
	 * @brief Put the message into a queue and notify sender
	 * to check queues for updates.
	 */
	void send(const GWMessage::Ptr message) override;

protected:
	/**
	 * @returns index of output queue to send from.
	 */
	size_t selectOutput() const;

	/**
	 * @breif Update status of output queues. The given index
	 * denotes queue that has been used to send a message.
	 */
	void updateOutputs(size_t i);

	/**
	 * @returns true if the queue of the given index is valid
	 * (existing queue)
	 */
	bool outputValid(size_t i) const;

	/**
	 * @returns first message in the queue of the given index
	 */
	GWMessage::Ptr peekOutput(size_t i) const;

	/**
	 * @brief Pop the first (oldest) message in the queue of the
	 * given index.
	 */
	void popOutput(size_t i);

protected:
	Poco::Event m_outputsUpdated;
	mutable Poco::Mutex m_outputLock;

private:
	unsigned int m_outputsCount;
	std::vector<std::queue<GWMessage::Ptr>> m_outputs;
	std::vector<size_t> m_outputsStatus;
	GWSPriorityAssigner::Ptr m_priorityAssigner;
};

}
