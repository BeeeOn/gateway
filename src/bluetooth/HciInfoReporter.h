#pragma once

#include <set>
#include <string>

#include <Poco/Mutex.h>
#include <Poco/Timespan.h>

#include "bluetooth/HciListener.h"
#include "bluetooth/HciInterface.h"
#include "hotplug/HotplugListener.h"
#include "loop/StoppableLoop.h"
#include "util/EventSource.h"
#include "util/Loggable.h"
#include "util/PeriodicRunner.h"

namespace BeeeOn {

/**
 * @brief HciInfoReporter periodically collects and reports statistics
 * information about HCI interfaces in the system. Reporting is done
 * via the HciListener interface.
 *
 * The HciInfoReporter depends on the hotplug system. It can serve any
 * bluetooth controller notified via the onAdd() event. Information about
 * multiple compatible HCI devices can be collected this way.
 *
 * Compatible HCI devices is recognized by method HciUtil::hotplugMatch().
 */
class HciInfoReporter :
	public StoppableLoop,
	public HotplugListener,
	Loggable {
public:
	HciInfoReporter();

	/**
	 * Set interval of periodic bluetooth statistics generation.
	 */
	void setStatisticsInterval(const Poco::Timespan &interval);

	/**
	 * Set HciInterfaceManager implementation.
	 */
	void setHciManager(HciInterfaceManager::Ptr manager);

	/**
	 * Set executor for delivering events.
	 */
	void setEventsExecutor(AsyncExecutor::Ptr executor);

	/**
	 * Register listener of bluetooth events.
	 */
	void registerListener(HciListener::Ptr listener);

	/**
	 * @brief Check the given event whether it is a bluetooth controller.
	 * Every matching controller is added to the list and statistics for
	 * it are reported periodically.
	 *
	 * @see HciUtil::hotplugMatch()
	 */
	void onAdd(const HotplugEvent &event) override;

	/**
	 * @brief Check the given event whether it is a bluetooth controller.
	 * The method allows to unplug a device and stop reporting for it.
	 *
	 * @see HciUtil::hotplugMatch()
	 */
	void onRemove(const HotplugEvent &event) override;

	/**
	 * @brief Start the periodic reporting loop.
	 */
	void start() override;

	/**
	 * @brief Stop the periodic reporting loop.
	 */
	void stop() override;

protected:
	std::set<std::string> dongles() const;

private:
	std::set<std::string> m_dongles;
	HciInterfaceManager::Ptr m_hciManager;
	PeriodicRunner m_statisticsRunner;
	EventSource<HciListener> m_eventSource;
	mutable Poco::FastMutex m_lock;
};

}
