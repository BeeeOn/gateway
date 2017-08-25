#include "zwave/ZWaveNodeEvent.h"

using namespace BeeeOn;

ZWaveNodeEvent::ZWaveNodeEvent(
		const OpenZWave::Node::NodeData &data, uint8_t nodeID):
	m_nodeID(nodeID),
	m_sentCnt(data.m_sentCnt),
	m_sentFailed(data.m_sentFailed),
	m_retries(data.m_retries),
	m_receivedCnt(data.m_receivedCnt),
	m_receivedDups(data.m_receivedDups),
	m_receivedUnsolicited(data.m_receivedUnsolicited),
	m_lastRequestRTT(data.m_lastRequestRTT),
	m_lastResponseRTT(data.m_lastResponseRTT),
	m_averageRequestRTT(data.m_averageRequestRTT),
	m_averageResponseRTT(data.m_averageResponseRTT),
	m_quality(data.m_quality)
{
}

uint32_t ZWaveNodeEvent::sentCount() const
{
	return m_sentCnt;
}

uint32_t ZWaveNodeEvent::sentFailed() const
{
	return m_sentFailed;
}

uint32_t ZWaveNodeEvent::retries() const
{
	return m_retries;
}

uint32_t ZWaveNodeEvent::receivedCount() const
{
	return m_receivedCnt;
}

uint32_t ZWaveNodeEvent::receiveDuplications() const
{
	return m_receivedDups;
}

uint32_t ZWaveNodeEvent::receiveUnsolicited() const
{
	return m_receivedUnsolicited;
}

uint32_t ZWaveNodeEvent::lastRequestRTT() const
{
	return m_lastRequestRTT;
}

uint32_t ZWaveNodeEvent::lastResponseRTT() const
{
	return m_lastResponseRTT;
}

uint32_t ZWaveNodeEvent::averageRequestRTT() const
{
	return m_averageRequestRTT;
}

uint32_t ZWaveNodeEvent::averageResponseRTT() const
{
	return m_averageResponseRTT;
}

uint32_t ZWaveNodeEvent::quality() const
{
	return m_quality;
}

uint8_t ZWaveNodeEvent::nodeID() const
{
	return m_nodeID;
}
