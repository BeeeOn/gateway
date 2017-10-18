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

void AbstractCollector::onHciStats(const HciInfo &)
{
}

void AbstractCollector::onDispatch(const Command::Ptr)
{
}
