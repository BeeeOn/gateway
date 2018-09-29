#include <Poco/Exception.h>

#include "di/Injectable.h"
#include "gwmessage/GWSensorDataConfirm.h"
#include "gwmessage/GWSensorDataExport.h"
#include "server/GWSQueuingExporter.h"

BEEEON_OBJECT_BEGIN(BeeeOn, GWSQueuingExporter)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_CASTABLE(GWSListener)
BEEEON_OBJECT_CASTABLE(Exporter)
BEEEON_OBJECT_PROPERTY("activeCount", &GWSQueuingExporter::setActiveCount)
BEEEON_OBJECT_PROPERTY("acquireTimeout", &GWSQueuingExporter::setAcquireTimeout)
BEEEON_OBJECT_PROPERTY("sendFailedDelay", &GWSQueuingExporter::setSendFailedDelay)
BEEEON_OBJECT_PROPERTY("connector", &GWSQueuingExporter::setConnector)
BEEEON_OBJECT_PROPERTY("queuingStrategy", &GWSQueuingExporter::setStrategy)
BEEEON_OBJECT_PROPERTY("saveThreshold", &GWSQueuingExporter::setSaveThreshold)
BEEEON_OBJECT_PROPERTY("saveTimeout", &GWSQueuingExporter::setSaveTimeout)
BEEEON_OBJECT_PROPERTY("strategyPriority", &GWSQueuingExporter::setStrategyPriority)
BEEEON_OBJECT_END(BeeeOn, GWSQueuingExporter)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

GWSQueuingExporter::GWSQueuingExporter():
	m_activeCount(10),
	m_acquireTimeout(5 * Timespan::SECONDS),
	m_sendFailedDelay(5 * Timespan::SECONDS)
{
}

void GWSQueuingExporter::setActiveCount(int count)
{
	if (count <= 0)
		throw InvalidArgumentException("activeCount must be positive");

	m_activeCount = count;
}

void GWSQueuingExporter::setAcquireTimeout(const Timespan &timeout)
{
	if (timeout < 0)
		throw InvalidArgumentException("acquireTimeout must not be negative");

	m_acquireTimeout = timeout;
}

void GWSQueuingExporter::setSendFailedDelay(const Timespan &delay)
{
	if (delay < 0)
		throw InvalidArgumentException("sendFailedDelay must not be negative");

	m_sendFailedDelay = delay;
}

void GWSQueuingExporter::setConnector(GWSConnector::Ptr connector)
{
	m_connector = connector;
}

void GWSQueuingExporter::run()
{
	StopControl::Run run(m_stopControl);

	logger().information("starting GWS queuing exporter");

	while (run) {
		vector<SensorData> active;

		acquire(active, m_activeCount, m_acquireTimeout);
		if (active.empty())
			continue;

		if (logger().trace()) {
			string details;

			for (const auto &data : active) {
				if (!details.empty())
					details += ", ";

				details += data.deviceID().toString();
				details += " (" + to_string(data.size()) + ")";
			}

			logger().trace(
				"exporting values: " + details,
				__FILE__, __LINE__);
		}

		const auto id = GlobalID::random();

		GWSensorDataExport::Ptr request = new GWSensorDataExport;
		request->setID(id);
		request->setData(active);

		try {
			m_connector->send(request);
		}
		BEEEON_CATCH_CHAIN_ACTION(logger(),
			m_stopControl.waitStoppable(m_sendFailedDelay);
			continue)
			

		bool acked = false;

		while (!acked && run) {
			m_event.wait();

			FastMutex::ScopedLock guard(m_ackedLock);
			acked = m_acked.find(id) != m_acked.end();
			m_acked.clear();
		}

		if (acked)
			ack();

		if (logger().debug()) {
			logger().debug(
				"recent request " + id.toString() + " has been acked",
				__FILE__, __LINE__);
		}
	}

	logger().information("GWS queuing exporter has stopped");
}

void GWSQueuingExporter::stop()
{
	m_stopControl.requestStop();
	m_event.set();
}

void GWSQueuingExporter::onConnected(const Address &)
{
	m_stopControl.requestWakeup();
}

void GWSQueuingExporter::onOther(const GWMessage::Ptr message)
{
	if (message.cast<GWSensorDataConfirm>().isNull())
		return;

	FastMutex::ScopedLock guard(m_ackedLock);
	m_acked.emplace(message->id());
	m_event.set();
}
