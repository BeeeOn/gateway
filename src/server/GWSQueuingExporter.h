#pragma once

#include <set>

#include <Poco/Event.h>
#include <Poco/Mutex.h>
#include <Poco/Timespan.h>

#include "core/QueuingExporter.h"
#include "loop/StoppableRunnable.h"
#include "loop/StopControl.h"
#include "server/GWSConnector.h"
#include "server/GWSListener.h"

namespace BeeeOn {

/**
 * @brief GWSQueuingExporter implements stop-and-go exporting logic based
 * on the QueuingExporter. The GWSQueuingExporter is to be explicitly
 * registered as a GWSListener to a selected GWSConnector instance.
 * The same GWSConnector instance should then be used for sending
 * of messages.
 *
 * GWSQueuingExporter exports data in batches (of size activeCount).
 * Each batch must be first confirmed from the Gateway Server before
 * another one is sent. This provides better reliablity and allows
 * to prevent data losses related to connection or power issues
 * (when the right QueuingStrategy is used).
 */
class GWSQueuingExporter :
	public QueuingExporter,
	public StoppableRunnable,
	public GWSListener {
public:
	typedef Poco::SharedPtr<GWSQueuingExporter> Ptr;

	GWSQueuingExporter();

	/**
	 * @brief Configure how many SensorData instances to acquire
	 * while exporting data, i.e. it denotes a batch size that
	 * is send via a single GWSensorDataExport message.
	 */
	void setActiveCount(int count);

	/**
	 * @brief Configure how long to wait until the QueuingExporter::acquire()
	 * operation returns a result. Tweaking the timeout an the activeCount
	 * property might lead to different performance.
	 */
	void setAcquireTimeout(const Poco::Timespan &timeout);

	/**
	 * @brief Configure delay for the next send attempt, if
	 * the current send() fails.
	 */
	void setSendFailedDelay(const Poco::Timespan &delay);

	/**
	 * @brief Configure GWSConnector instance to send data through.
	 */
	void setConnector(GWSConnector::Ptr connector);

	/**
	 * @brief Wake-up failed sending when it seems that the connection
	 * is up again.
	 */
	void onConnected(const Address &address) override;

	/**
	 * @brief Receive GWSensorDataConfirm messages via this method.
	 */
	void onOther(const GWMessage::Ptr message) override;

	void run() override;
	void stop() override;
	
private:
	size_t m_activeCount;
	Poco::Timespan m_acquireTimeout;
	Poco::Timespan m_sendFailedDelay;
	GWSConnector::Ptr m_connector;
	StopControl m_stopControl;
	Poco::Event m_event;
	std::set<GlobalID> m_acked;
	Poco::FastMutex m_ackedLock;
};

}
