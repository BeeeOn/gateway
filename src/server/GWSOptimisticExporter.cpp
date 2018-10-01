#include <Poco/Exception.h>

#include "di/Injectable.h"
#include "gwmessage/GWSensorDataExport.h"
#include "server/GWSOptimisticExporter.h"

BEEEON_OBJECT_BEGIN(BeeeOn, GWSOptimisticExporter)
BEEEON_OBJECT_CASTABLE(Exporter)
BEEEON_OBJECT_CASTABLE(GWSListener)
BEEEON_OBJECT_PROPERTY("connector", &GWSOptimisticExporter::setConnector)
BEEEON_OBJECT_PROPERTY("exportNonConfirmed", &GWSOptimisticExporter::setExportNonConfirmed)
BEEEON_OBJECT_END(BeeeOn, GWSOptimisticExporter)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

GWSOptimisticExporter::GWSOptimisticExporter():
	m_exportNonConfirmed(1),
	m_connected(false)
{
}

void GWSOptimisticExporter::setConnector(GWSConnector::Ptr connector)
{
	m_connector = connector;
}

void GWSOptimisticExporter::setExportNonConfirmed(int count)
{
	if (count < 1)
		throw InvalidArgumentException("exportNonConfirmed must be at least 1");

	m_exportNonConfirmed = count;
}

bool GWSOptimisticExporter::ship(const SensorData &data)
{
	if (!m_connected)
		return false;

	FastMutex::ScopedLock guard(m_lock);

	if (m_exported.size() >= m_exportNonConfirmed)
		return false;

	const auto id = GlobalID::random();

	GWSensorDataExport::Ptr request = new GWSensorDataExport;
	request->setID(id);
	request->setData({data});

	if (logger().debug()) {
		logger().debug(
			"exporting " + to_string(data.size())
			+ " values for device " + data.deviceID().toString(),
			__FILE__, __LINE__);
	}

	try {
		m_connector->send(request);
		m_exported.emplace(id);
		return true;
	}
	BEEEON_CATCH_CHAIN_ACTION(logger(),
		return false)
}

void GWSOptimisticExporter::onOther(const GWMessage::Ptr message)
{
	if (message.cast<GWSensorDataConfirm>().isNull())
		return;

	FastMutex::ScopedLock guard(m_lock);

	m_exported.erase(message->id());
}

void GWSOptimisticExporter::onConnected(const GWSListener::Address &)
{
	m_connected = true;
}

void GWSOptimisticExporter::onDisconnected(const GWSListener::Address &)
{
	m_connected = false;
}
