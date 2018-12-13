#include "core/PollingKeeper.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

PollingKeeper::PollingKeeper()
{
}

PollingKeeper::~PollingKeeper()
{
	cancelAll();
}

void PollingKeeper::setDevicePoller(DevicePoller::Ptr poller)
{
	m_devicePoller = poller;
}

void PollingKeeper::schedule(PollableDevice::Ptr device)
{
	FastMutex::ScopedLock guard(m_lock);

	m_devicePoller->schedule(device);
	m_polled.emplace(device->id(), device);
}

void PollingKeeper::cancel(const DeviceID &id)
{
	FastMutex::ScopedLock guard(m_lock);

	m_polled.erase(id);
	m_devicePoller->cancel(id);
}

void PollingKeeper::cancelAll()
{
	FastMutex::ScopedLock guard(m_lock);

	for (const auto &pair : m_polled)
		m_devicePoller->cancel(pair.first);

	m_polled.clear();
}
