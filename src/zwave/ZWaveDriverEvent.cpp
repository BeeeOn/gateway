#include "zwave/ZWaveDriverEvent.h"

using namespace BeeeOn;

ZWaveDriverEvent::ZWaveDriverEvent(const OpenZWave::Driver::DriverData &data):
	m_SOFCnt(data.m_SOFCnt),
	m_ACKWaiting(data.m_ACKWaiting),
	m_readAborts(data.m_readAborts),
	m_badChecksum(data.m_badChecksum),
	m_readCnt(data.m_readCnt),
	m_writeCnt(data.m_writeCnt),
	m_CANCnt(data.m_CANCnt),
	m_NAKCnt(data.m_NAKCnt),
	m_ACKCnt(data.m_ACKCnt),
	m_OOFCnt(data.m_OOFCnt),
	m_dropped(data.m_dropped),
	m_retries(data.m_retries),
	m_callbacks(data.m_callbacks),
	m_badroutes(data.m_badroutes),
	m_noACK(data.m_noack),
	m_netbusy(data.m_netbusy),
	m_notidle(data.m_notidle),
	m_nondelivery(data.m_nondelivery),
	m_routedbusy(data.m_routedbusy),
	m_broadcastReadCnt(data.m_broadcastReadCnt),
	m_broadcastWriteCnt(data.m_broadcastWriteCnt)
{
}

uint32_t ZWaveDriverEvent::SOAFCount() const
{
	return m_SOFCnt;
}

uint32_t ZWaveDriverEvent::ACKWaiting() const
{
	return m_ACKWaiting;
}

uint32_t ZWaveDriverEvent::readAborts() const
{
	return m_readAborts;
}

uint32_t ZWaveDriverEvent::badChecksum() const
{
	return m_badChecksum;
}

uint32_t ZWaveDriverEvent::readCount() const
{
	return m_readCnt;
}

uint32_t ZWaveDriverEvent::writeCount() const
{
	return m_writeCnt;
}

uint32_t ZWaveDriverEvent::CANCount() const
{
	return m_CANCnt;
}

uint32_t ZWaveDriverEvent::NAKCount() const
{
	return m_NAKCnt;
}

uint32_t ZWaveDriverEvent::ACKCount() const
{
	return m_ACKCnt;
}

uint32_t ZWaveDriverEvent::OOFCount() const
{
	return m_OOFCnt;
}

uint32_t ZWaveDriverEvent::dropped() const
{
	return m_dropped;
}

uint32_t ZWaveDriverEvent::retries() const
{
	return m_retries;
}

uint32_t ZWaveDriverEvent::callbacks() const
{
	return m_callbacks;
}

uint32_t ZWaveDriverEvent::badroutes() const
{
	return m_badroutes;
}

uint32_t ZWaveDriverEvent::noACK() const
{
	return m_noACK;
}

uint32_t ZWaveDriverEvent::netBusy() const
{
	return m_netbusy;
}

uint32_t ZWaveDriverEvent::notIdle() const
{
	return m_notidle;
}

uint32_t ZWaveDriverEvent::nonDelivery() const
{
	return m_nondelivery;
}

uint32_t ZWaveDriverEvent::routedBusy() const
{
	return m_routedbusy;
}

uint32_t ZWaveDriverEvent::broadcastReadCount() const
{
	return m_broadcastReadCnt;
}

uint32_t ZWaveDriverEvent::broadcastWriteCount() const
{
	return m_broadcastWriteCnt;
}
