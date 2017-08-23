#ifndef BEEEON_ABSTRACT_HOTPLUG_MONITOR_H
#define BEEEON_ABSTRACT_HOTPLUG_MONITOR_H

#include <list>
#include <string>

#include "hotplug/HotplugListener.h"
#include "util/Loggable.h"

namespace BeeeOn {

class HotplugEvent;

class AbstractHotplugMonitor : protected Loggable {
public:
	void registerListener(HotplugListener::Ptr listener);

protected:
	AbstractHotplugMonitor();

	void logEvent(const HotplugEvent &event, const std::string &action) const;
	void fireAddEvent(const HotplugEvent &event);
	void fireRemoveEvent(const HotplugEvent &event);
	void fireChangeEvent(const HotplugEvent &event);
	void fireMoveEvent(const HotplugEvent &event);

private:
	std::list<HotplugListener::Ptr> m_listeners;
};

}

#endif
