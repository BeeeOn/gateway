#include "core/AbstractCollector.h"

using namespace BeeeOn;

AbstractCollector::~AbstractCollector()
{
}

void AbstractCollector::onExport(const SensorData &)
{
}

void AbstractCollector::onDriverStats(const ZWaveDriverEvent &)
{
}

void AbstractCollector::onNodeStats(const ZWaveNodeEvent &)
{
}

void AbstractCollector::onNotification(const OZWNotificationEvent &)
{
}

void AbstractCollector::onHciStats(const HciInfo &)
{
}

void AbstractCollector::onBulbStats(const PhilipsHueBulbInfo &)
{
}

void AbstractCollector::onBridgeStats(const PhilipsHueBridgeInfo &)
{
}

void AbstractCollector::onDispatch(const Command::Ptr)
{
}

void AbstractCollector::onReceiveDPA(const IQRFEvent &)
{
}
