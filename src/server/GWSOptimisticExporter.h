#pragma once

#include <set>

#include <Poco/AtomicCounter.h>
#include <Poco/Mutex.h>

#include "core/Exporter.h"
#include "gwmessage/GWMessage.h"
#include "server/GWSConnector.h"
#include "server/GWSListener.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief GWSOptimisticExporter implements exporting via GWSConnector.
 * It wraps the given SensorData instances and pass them to the
 * GWSConnector::send(). It also keeps track of the connectivity
 * to the remote server. Exporting is implemented optimistically,
 * we assume no network failures. If the number of non-confirmed
 * exports reached the limit exportNonConfirmed, no more exports
 * occures until a confirmation comes.
 */
class GWSOptimisticExporter :
	public Exporter,
	public GWSListener,
	Loggable {
public:
	typedef Poco::SharedPtr<GWSOptimisticExporter> Ptr;

	GWSOptimisticExporter();

	void setConnector(GWSConnector::Ptr connector);
	void setExportNonConfirmed(int count);

	/**
	 * @brief Ship the given data via GWSConnector::send() to the
	 * remote server. The connectivity status of the GWSConnector
	 * is considered.
	 *
	 * @returns true if GWSConnector::send() succeeded, false if
	 * there is no connectivity, no confirmations for the configured
	 * number of outstanding exports or the GWSConnector::send() fails.
	 */
	bool ship(const SensorData &data) override;

	/**
	 * @brief Process confirmations of exported data.
	 */
	void onOther(const GWMessage::Ptr message) override;

	/**
	 * @brief Notice that the GWSConnector is connected.
	 */
	void onConnected(const GWSListener::Address &) override;

	/**
	 * @brief Notice that the GWSConnector is disconnected.
	 */
	void onDisconnected(const GWSListener::Address &) override;

private:
	size_t m_exportNonConfirmed;
	GWSConnector::Ptr m_connector;
	Poco::AtomicCounter m_connected;
	std::set<GlobalID> m_exported;
	Poco::FastMutex m_lock;
};

}
