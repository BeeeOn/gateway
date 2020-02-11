#include "conrad/FHEMDeviceInfo.h"

using namespace std;
using namespace BeeeOn;
using namespace Poco;

FHEMDeviceInfo::FHEMDeviceInfo(
		const string& dev,
		const uint32_t protRcv,
		const uint32_t protSnd,
		const Timestamp& lastRcv):
			m_dev(dev),
			m_protRcv(protRcv),
			m_protSnd(protSnd),
			m_lastRcv(lastRcv)
{
}

string FHEMDeviceInfo::dev() const
{
	return m_dev;
}

uint32_t FHEMDeviceInfo::protRcv() const
{
	return m_protRcv;
}

void FHEMDeviceInfo::setProtRcv(const uint32_t protRcv)
{
	m_protRcv = protRcv;
}

uint32_t FHEMDeviceInfo::protSnd() const
{
	return m_protSnd;
}

void FHEMDeviceInfo::setProtSnd(const uint32_t protSnd)
{
	m_protSnd = protSnd;
}

Timestamp FHEMDeviceInfo::lastRcv() const
{
	return m_lastRcv;
}

void FHEMDeviceInfo::setLastRcv(const Timestamp& lastRcv)
{
	m_lastRcv = lastRcv;
}
