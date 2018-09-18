#include <Poco/Exception.h>
#include "zwave/ZWaveDriverEvent.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

ZWaveDriverEvent::ZWaveDriverEvent(const map<string, uint32_t> &stats):
	m_stats(stats)
{
}

uint32_t ZWaveDriverEvent::lookup(const string &key) const
{
	auto it = m_stats.find(key);
	if (it == m_stats.end())
		throw NotFoundException("no such driver statistic " + key);

	return it->second;
}

uint32_t ZWaveDriverEvent::SOAFCount() const
{
	return lookup("SOFCnt");
}

uint32_t ZWaveDriverEvent::ACKWaiting() const
{
	return lookup("ACKWaiting");
}

uint32_t ZWaveDriverEvent::readAborts() const
{
	return lookup("readAborts");
}

uint32_t ZWaveDriverEvent::badChecksum() const
{
	return lookup("badChecksum");
}

uint32_t ZWaveDriverEvent::readCount() const
{
	return lookup("readCnt");
}

uint32_t ZWaveDriverEvent::writeCount() const
{
	return lookup("writeCnt");
}

uint32_t ZWaveDriverEvent::CANCount() const
{
	return lookup("CANCnt");
}

uint32_t ZWaveDriverEvent::NAKCount() const
{
	return lookup("NAKCnt");
}

uint32_t ZWaveDriverEvent::ACKCount() const
{
	return lookup("ACKCnt");
}

uint32_t ZWaveDriverEvent::OOFCount() const
{
	return lookup("OOFCnt");
}

uint32_t ZWaveDriverEvent::dropped() const
{
	return lookup("dropped");
}

uint32_t ZWaveDriverEvent::retries() const
{
	return lookup("retries");
}

uint32_t ZWaveDriverEvent::callbacks() const
{
	return lookup("callbacks");
}

uint32_t ZWaveDriverEvent::badroutes() const
{
	return lookup("badroutes");
}

uint32_t ZWaveDriverEvent::noACK() const
{
	return lookup("noACK");
}

uint32_t ZWaveDriverEvent::netBusy() const
{
	return lookup("netbusy");
}

uint32_t ZWaveDriverEvent::notIdle() const
{
	return lookup("notidle");
}

uint32_t ZWaveDriverEvent::nonDelivery() const
{
	return lookup("nondelivery");
}

uint32_t ZWaveDriverEvent::routedBusy() const
{
	return lookup("routedbusy");
}

uint32_t ZWaveDriverEvent::broadcastReadCount() const
{
	return lookup("broadcastReadCnt");
}

uint32_t ZWaveDriverEvent::broadcastWriteCount() const
{
	return lookup("broadcastWriteCnt");
}
