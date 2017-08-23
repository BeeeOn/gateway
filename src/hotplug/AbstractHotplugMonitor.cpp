#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/LogStream.h>

#include "hotplug/AbstractHotplugMonitor.h"
#include "hotplug/HotplugEvent.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

AbstractHotplugMonitor::AbstractHotplugMonitor()
{
}

void AbstractHotplugMonitor::registerListener(HotplugListener::Ptr listener)
{
	m_listeners.push_back(listener);
}

void AbstractHotplugMonitor::fireAddEvent(const HotplugEvent &event)
{
	for (auto &listener : m_listeners) {
		try {
			listener->onAdd(event);
		}
		catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
		}
		catch (const exception &e) {
			logger().critical(e.what(), __FILE__, __LINE__);
		}
		catch (...) {
			logger().critical("unknown error", __FILE__, __LINE__);
		}
	}
}

void AbstractHotplugMonitor::fireRemoveEvent(const HotplugEvent &event)
{
	for (auto &listener : m_listeners) {
		try {
			listener->onRemove(event);
		}
		catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
		}
		catch (const exception &e) {
			logger().critical(e.what(), __FILE__, __LINE__);
		}
		catch (...) {
			logger().critical("unknown error", __FILE__, __LINE__);
		}
	}
}

void AbstractHotplugMonitor::fireChangeEvent(const HotplugEvent &event)
{
	for (auto &listener : m_listeners) {
		try {
			listener->onChange(event);
		}
		catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
		}
		catch (const exception &e) {
			logger().critical(e.what(), __FILE__, __LINE__);
		}
		catch (...) {
			logger().critical("unknown error", __FILE__, __LINE__);
		}
	}
}

void AbstractHotplugMonitor::fireMoveEvent(const HotplugEvent &event)
{
	for (auto &listener : m_listeners) {
		try {
			listener->onMove(event);
		}
		catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
		}
		catch (const exception &e) {
			logger().critical(e.what(), __FILE__, __LINE__);
		}
		catch (...) {
			logger().critical("unknown error", __FILE__, __LINE__);
		}
	}
}

void AbstractHotplugMonitor::logEvent(
		const HotplugEvent &event,
		const string &action) const
{
	logger().debug("device event " + event.toString()
		       + " (" + action + ")",
		       __FILE__, __LINE__);

	if (logger().trace()) {
		LogStream stream(logger(), Message::PRIO_TRACE);
		event.properties()->save(stream);
	}
}

