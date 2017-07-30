#ifndef BEEEON_UDEV_MONITOR_H
#define BEEEON_UDEV_MONITOR_H

#include <list>
#include <set>
#include <string>

#include <Poco/AtomicCounter.h>
#include <Poco/Timespan.h>

#include "hotplug/HotplugListener.h"
#include "loop/StoppableRunnable.h"
#include "util/Loggable.h"

struct udev;
struct udev_device;
struct udev_monitor;

namespace BeeeOn {

class UDevMonitor : public StoppableRunnable, Loggable {
public:
	UDevMonitor();
	~UDevMonitor();

	void run() override;
	void stop() override;

	void setMatches(const std::list<std::string> &matches);
	void setPollTimeout(const int ms);
	void setIncludeParents(bool enable);
	void registerListener(HotplugListener::Ptr listener);

	void initialScan();

private:
	struct udev_monitor *createMonitor();
	struct udev_monitor *doCreateMonitor();
	void collectProperties(
		HotplugEvent::Properties &event, struct udev_device *dev) const;
	HotplugEvent createEvent(struct udev_device *dev) const;
	void scanDevice(struct udev_monitor *mon);
	void logEvent(const HotplugEvent &event, const std::string &action) const;
	void throwFromErrno(const std::string &name);

	void fireAddEvent(const HotplugEvent &event);
	void fireRemoveEvent(const HotplugEvent &event);
	void fireChangeEvent(const HotplugEvent &event);
	void fireMoveEvent(const HotplugEvent &event);

private:
	std::list<HotplugListener::Ptr> m_listeners;
	std::set<std::string> m_matches;
	Poco::AtomicCounter m_stop;
	Poco::Timespan m_pollTimeout;
	bool m_includeParents;
	struct udev *m_udev;
};

}

#endif
