#include <Poco/Exception.h>

#include "zwave/ZWaveNodeEvent.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

ZWaveNodeEvent::ZWaveNodeEvent(
		const map<string, uint32_t> &stats, uint8_t nodeID):
	m_nodeID(nodeID),
	m_stats(stats)
{
}

uint32_t ZWaveNodeEvent::lookup(const string &key) const
{
	auto it = m_stats.find(key);
	if (it == m_stats.end())
		throw NotFoundException("no such node statistic " + key);

	return it->second;
}

uint32_t ZWaveNodeEvent::sentCount() const
{
	return lookup("sentCnt");
}

uint32_t ZWaveNodeEvent::sentFailed() const
{
	return lookup("sentFailed");
}

uint32_t ZWaveNodeEvent::retries() const
{
	return lookup("retries");
}

uint32_t ZWaveNodeEvent::receivedCount() const
{
	return lookup("receivedCnt");
}

uint32_t ZWaveNodeEvent::receiveDuplications() const
{
	return lookup("receivedDups");
}

uint32_t ZWaveNodeEvent::receiveUnsolicited() const
{
	return lookup("receivedUnsolicited");
}

uint32_t ZWaveNodeEvent::lastRequestRTT() const
{
	return lookup("lastRequestRTT");
}

uint32_t ZWaveNodeEvent::lastResponseRTT() const
{
	return lookup("lastResponseRTT");
}

uint32_t ZWaveNodeEvent::averageRequestRTT() const
{
	return lookup("averageRequestRTT");
}

uint32_t ZWaveNodeEvent::averageResponseRTT() const
{
	return lookup("averageResponseRTT");
}

uint32_t ZWaveNodeEvent::quality() const
{
	return lookup("quality");
}

uint8_t ZWaveNodeEvent::nodeID() const
{
	return m_nodeID;
}
