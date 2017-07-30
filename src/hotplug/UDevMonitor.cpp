#include <cerrno>
#include <cstring>

#include <libudev.h>
#include <poll.h>

#include <Poco/Exception.h>
#include <Poco/Logger.h>
#include <Poco/LogStream.h>
#include <Poco/Thread.h>

#include "di/Injectable.h"
#include "hotplug/UDevEvent.h"
#include "hotplug/UDevMonitor.h"

BEEEON_OBJECT_BEGIN(BeeeOn, UDevMonitor)
BEEEON_OBJECT_CASTABLE(StoppableRunnable)
BEEEON_OBJECT_LIST("matches", &UDevMonitor::setMatches)
BEEEON_OBJECT_NUMBER("pollTimeout", &UDevMonitor::setPollTimeout)
BEEEON_OBJECT_NUMBER("includeParents", &UDevMonitor::setIncludeParents)
BEEEON_OBJECT_REF("listeners", &UDevMonitor::registerListener)
BEEEON_OBJECT_HOOK("done", &UDevMonitor::initialScan)
BEEEON_OBJECT_END(BeeeOn, UDevMonitor)

using namespace std;
using namespace Poco;
using namespace BeeeOn;

UDevMonitor::UDevMonitor():
	m_stop(0),
	m_pollTimeout(1000 * Timespan::MILLISECONDS),
	m_includeParents(false),
	m_udev(NULL)
{
	m_udev = ::udev_new();
	if (m_udev == NULL)
		throwFromErrno("udev_new");
}

UDevMonitor::~UDevMonitor()
{
	if (m_udev != NULL)
		::udev_unref(m_udev);
}

void UDevMonitor::throwFromErrno(const string &name)
{
	if (errno) {
		throw SystemException(name + ": " + ::strerror(errno));
	}

	throw IOException(name + " has failed");
}

void UDevMonitor::setMatches(const list<string> &matches)
{
	for (const auto &m : matches)
		m_matches.emplace(m);
}

void UDevMonitor::setPollTimeout(const int ms)
{
	// we do not check the ms value because:
	// * poll(..., 0) is non-blocking
	// * poll(..., negative) is blocking
	// * poll(..., positive) is blocking with timeout

	m_pollTimeout = Timespan(ms * Timespan::MILLISECONDS);
}

void UDevMonitor::setIncludeParents(bool enable)
{
	m_includeParents = enable;
}

void UDevMonitor::registerListener(UDevListener::Ptr listener)
{
	m_listeners.push_back(listener);
}

void UDevMonitor::fireAddEvent(const UDevEvent &event)
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

void UDevMonitor::fireRemoveEvent(const UDevEvent &event)
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

void UDevMonitor::fireChangeEvent(const UDevEvent &event)
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

void UDevMonitor::fireMoveEvent(const UDevEvent &event)
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

void UDevMonitor::collectProperties(
	UDevEvent::Properties &properties, struct udev_device *dev) const
{
	const char *subsystem = ::udev_device_get_subsystem(dev);
	const string &prefix = string(subsystem ? subsystem : "") + (subsystem ? "." : "");

	struct udev_list_entry *attributes;
	struct udev_list_entry *current;

	attributes = ::udev_device_get_properties_list_entry(dev);
	udev_list_entry_foreach(current, attributes) {
		const char *name = ::udev_list_entry_get_name(current);
		if (name == NULL)
			continue;

		const char *value = ::udev_device_get_property_value(dev, name);
		properties.setString(prefix + name, value ? value : "");
	}
}

UDevEvent UDevMonitor::createEvent(struct udev_device *dev) const
{
	const char *subsystem = ::udev_device_get_subsystem(dev);
	UDevEvent event;

	const char *node = ::udev_device_get_devnode(dev);
	const char *type = ::udev_device_get_devtype(dev);
	const char *sysname = ::udev_device_get_sysname(dev);
	const char *driver = ::udev_device_get_driver(dev);

	event.setSubsystem(subsystem ? subsystem : "");
	event.setNode(node ? node : "");
	event.setType(type ? type : "");
	event.setName(sysname ? sysname : "");
	event.setDriver(driver ? driver : "");

	if (m_includeParents) {
		struct udev_device *current = dev;
		while ((current = ::udev_device_get_parent(current)) != NULL)
			collectProperties(*event.properties(), current);
	}

	collectProperties(*event.properties(), dev);

	return event;
}

void UDevMonitor::logEvent(const UDevEvent &event, const string &action) const
{
	logger().debug("device event " + event.toString()
		       + " (" + action + ")",
		       __FILE__, __LINE__);

	if (logger().trace()) {
		LogStream stream(logger(), Message::PRIO_TRACE);
		event.properties()->save(stream);
	}
}

void UDevMonitor::initialScan()
{
	logger().information("initial subsystem udev scan", __FILE__, __LINE__);

	struct udev_enumerate *en = ::udev_enumerate_new(m_udev);
	if (en == NULL)
		throwFromErrno("udev_enumerate_new");

	for (const auto &m : m_matches) {
		int ret = ::udev_enumerate_add_match_subsystem(en, m.c_str());
		if (ret < 0) {
			::udev_enumerate_unref(en);
			throwFromErrno("udev_enumerate_add_match_subsystem");
		}
	}

	int ret = ::udev_enumerate_scan_devices(en);
	if (ret < 0) {
		::udev_enumerate_unref(en);
		throwFromErrno("udev_enumerate_scan_devices");
	}

	struct udev_list_entry *devices = NULL;
	struct udev_list_entry *current = NULL;
	devices = ::udev_enumerate_get_list_entry(en);

	udev_list_entry_foreach(current, devices) {
		const char *syspath = ::udev_list_entry_get_name(current);
		if (syspath == NULL) {
			logger().critical("no syspath for udev entry",
					  __FILE__, __LINE__);
		}

		struct udev_device *dev;
		dev = ::udev_device_new_from_syspath(m_udev, syspath);
		if (dev == NULL) {
			::udev_enumerate_unref(en);
			throwFromErrno("udev_device_new_from_syspath");
		}

		const UDevEvent &event = createEvent(dev);
		::udev_device_unref(dev);

		logEvent(event, "initial");

		fireAddEvent(event);
	}

	::udev_enumerate_unref(en);
}

void UDevMonitor::scanDevice(struct udev_monitor *mon)
{
	int fd = ::udev_monitor_get_fd(mon);
	struct pollfd pollfd = {fd, POLLIN, 0};

	int ret = poll(&pollfd, 1, m_pollTimeout.totalMilliseconds());
	if (ret < 0)
		throwFromErrno("poll");
	else if (ret == 0)
		return;

	struct udev_device *dev = ::udev_monitor_receive_device(mon);
	if (dev == NULL) {
		if (errno) {
			logger().warning(
				string("udev_monitor_receive_device: ") +
					 ::strerror(errno),
				__FILE__, __LINE__
			);
		}

		return;
	}

	const char *action = ::udev_device_get_action(dev);

	if (action == NULL) {
		::udev_device_unref(dev);
		throwFromErrno("udev_device_get_action");
	}

	const UDevEvent &event = createEvent(dev);
	logEvent(event, action);

	if (!::strcmp(action, "add"))
		fireAddEvent(event);
	else if (!::strcmp(action, "remove"))
		fireRemoveEvent(event);
	else if (!::strcmp(action, "change"))
		fireChangeEvent(event);
	else if (!::strcmp(action, "move"))
		fireMoveEvent(event);

	::udev_device_unref(dev);
}

struct udev_monitor *UDevMonitor::doCreateMonitor()
{
	struct udev_monitor *mon = ::udev_monitor_new_from_netlink(m_udev, "udev");
	if (mon == NULL)
		throwFromErrno("udev_monitor_new_from_netlink");

	for (const auto &m : m_matches) {
		int ret = ::udev_monitor_filter_add_match_subsystem_devtype(
				mon, m.c_str(), NULL);
		if (ret < 0) {
			::udev_monitor_unref(mon);
			throwFromErrno("udev_monitor_filter_add_match_subsystem_devtype");
		}
	}

	if (::udev_monitor_enable_receiving(mon) < 0) {
		::udev_monitor_unref(mon);
		throwFromErrno("udev_monitor_enable_receiving");
	}

	return mon;
}

struct udev_monitor *UDevMonitor::createMonitor()
{
	try {
		return doCreateMonitor();
	}
	catch (const Exception &e) {
		logger().log(e, __FILE__, __LINE__);
	}
	catch (const exception &e) {
		logger().critical(e.what(), __FILE__, __LINE__);
	}
	catch (...) {
		logger().critical("failed to create udev_monitor",
				  __FILE__, __LINE__);
	}

	return NULL;
}

void UDevMonitor::run()
{
	struct udev_monitor *mon = createMonitor();
	if (mon == NULL) {
		logger().critical("leaving udev monitoring early",
				 __FILE__, __LINE__);

		return;
	}

	logger().information("start udev monitoring",
			     __FILE__, __LINE__);

	while (!m_stop) {
		try {
			scanDevice(mon);
		}
		catch (const Exception &e) {
			logger().log(e, __FILE__, __LINE__);
		}
		catch (const exception &e) {
			logger().critical(e.what(), __FILE__, __LINE__);
		}
		catch (...) {
			logger().critical("failed to scan devices",
				  __FILE__, __LINE__);
		}
	}

	::udev_monitor_unref(mon);

	logger().information("stop udev monitoring",
			     __FILE__, __LINE__);

	m_stop = false;
}

void UDevMonitor::stop()
{
	m_stop = true;
}
