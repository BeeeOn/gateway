#include <Poco/Format.h>

#include "hotplug/HotplugEvent.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

HotplugEvent::HotplugEvent():
	m_properties(new Properties)
{
}

void HotplugEvent::setSubsystem(const string &subsystem)
{
	m_subsystem = subsystem;
}

string HotplugEvent::subsystem() const
{
	return m_subsystem;
}

void HotplugEvent::setNode(const string &node)
{
	m_node = node;
}

string HotplugEvent::node() const
{
	return m_node;
}

void HotplugEvent::setType(const string &type)
{
	m_type = type;
}

string HotplugEvent::type() const
{
	return m_type;
}

void HotplugEvent::setName(const string &name)
{
	m_name = name;
}

string HotplugEvent::name() const
{
	return m_name;
}

void HotplugEvent::setDriver(const string &driver)
{
	m_driver = driver;
}

string HotplugEvent::driver() const
{
	return m_driver;
}

string HotplugEvent::toString() const
{
	return Poco::format("%s,%s,%s,%s,%s",
		subsystem(), node(), type(), name(), driver());
}

AutoPtr<HotplugEvent::Properties> HotplugEvent::properties() const
{
	return m_properties;
}
