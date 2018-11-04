#include <Poco/Exception.h>
#include <Poco/Logger.h>

#include "di/Injectable.h"
#include "bluetooth/HciInfoReporter.h"
#include "bluetooth/HciUtil.h"

BEEEON_OBJECT_BEGIN(BeeeOn, HciInfoReporter)
BEEEON_OBJECT_CASTABLE(StoppableLoop)
BEEEON_OBJECT_CASTABLE(HotplugListener)
BEEEON_OBJECT_PROPERTY("statisticsInterval", &HciInfoReporter::setStatisticsInterval)
BEEEON_OBJECT_PROPERTY("hciManager", &HciInfoReporter::setHciManager)
BEEEON_OBJECT_PROPERTY("eventsExecutor", &HciInfoReporter::setEventsExecutor)
BEEEON_OBJECT_PROPERTY("listeners", &HciInfoReporter::registerListener)
BEEEON_OBJECT_END(BeeeOn, HciInfoReporter)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

HciInfoReporter::HciInfoReporter()
{
}

void HciInfoReporter::setStatisticsInterval(const Timespan &interval)
{
	if (interval <= 0)
		throw InvalidArgumentException("statisticsInterval must be a positive number");

	m_statisticsRunner.setInterval(interval);
}

void HciInfoReporter::setHciManager(HciInterfaceManager::Ptr manager)
{
	m_hciManager = manager;
}

void HciInfoReporter::setEventsExecutor(AsyncExecutor::Ptr executor)
{
	m_eventSource.setAsyncExecutor(executor);
}

void HciInfoReporter::registerListener(HciListener::Ptr listener)
{
	m_eventSource.addListener(listener);
}

void HciInfoReporter::onAdd(const HotplugEvent &event)
{
	const auto name = HciUtil::hotplugMatch(event);
	if (name.empty())
		return;

	FastMutex::ScopedLock guard(m_lock);

	if (m_dongles.find(name) == m_dongles.end()) {
		m_dongles.emplace(name);

		logger().information(
			"start reporting statistics for: " + name,
			__FILE__, __LINE__);
	}
}

void HciInfoReporter::onRemove(const HotplugEvent &event)
{
	const auto name = HciUtil::hotplugMatch(event);
	if (name.empty())
		return;

	FastMutex::ScopedLock guard(m_lock);

	if (m_dongles.find(name) == m_dongles.end()) {
		m_dongles.erase(name);

		logger().information(
			"stop reporting statistics for: " + name,
			__FILE__, __LINE__);
	}
}

void HciInfoReporter::start()
{
	m_statisticsRunner.start([&]() {
		for (const auto &name : dongles()) {
			if (logger().debug()) {
				logger().debug(
					"reporting HCI statistics for " + name,
					__FILE__, __LINE__);
			}

			try {

				HciInterface::Ptr hci = m_hciManager->lookup(name);
				const HciInfo &info = hci->info();
				m_eventSource.fireEvent(info, &HciListener::onHciStats);
			}
			BEEEON_CATCH_CHAIN(logger())
		}
	});
}

void HciInfoReporter::stop()
{
	m_statisticsRunner.stop();
}

set<string> HciInfoReporter::dongles() const
{
	FastMutex::ScopedLock guard(m_lock);
	return m_dongles;
}
