#ifndef BEEEON_UDEV_MONITOR_H
#define BEEEON_UDEV_MONITOR_H

#include <list>
#include <set>
#include <string>

#include <Poco/AtomicCounter.h>
#include <Poco/Timespan.h>

#include "hotplug/AbstractHotplugMonitor.h"
#include "loop/StoppableRunnable.h"

struct udev;
struct udev_device;
struct udev_monitor;

namespace BeeeOn {

class UDevMonitor : public StoppableRunnable, public AbstractHotplugMonitor {
public:
	UDevMonitor();
	~UDevMonitor();

	void run() override;
	void stop() override;

	void setMatches(const std::list<std::string> &matches);
	void setPollTimeout(const Poco::Timespan &timeout);
	void setIncludeParents(bool enable);

	void initialScan();

private:
	struct udev_monitor *createMonitor();
	struct udev_monitor *doCreateMonitor();
	void collectProperties(
		HotplugEvent::Properties &event, struct udev_device *dev) const;
	HotplugEvent createEvent(struct udev_device *dev) const;
	void scanDevice(struct udev_monitor *mon);
	void throwFromErrno(const std::string &name);

private:
	std::set<std::string> m_matches;
	Poco::AtomicCounter m_stop;
	Poco::Timespan m_pollTimeout;
	bool m_includeParents;
	struct udev *m_udev;
};

}

#endif
