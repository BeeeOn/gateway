#include <Poco/Format.h>

#include "hotplug/UDevEvent.h"

using namespace std;
using namespace Poco;
using namespace BeeeOn;

UDevEvent::UDevEvent():
	m_properties(new Properties)
{
}

void UDevEvent::setSubsystem(const string &subsystem)
{
	m_subsystem = subsystem;
}

string UDevEvent::subsystem() const
{
	return m_subsystem;
}

void UDevEvent::setNode(const string &node)
{
	m_node = node;
}

string UDevEvent::node() const
{
	return m_node;
}

void UDevEvent::setType(const string &type)
{
	m_type = type;
}

string UDevEvent::type() const
{
	return m_type;
}

void UDevEvent::setName(const string &name)
{
	m_name = name;
}

string UDevEvent::name() const
{
	return m_name;
}

void UDevEvent::setDriver(const string &driver)
{
	m_driver = driver;
}

string UDevEvent::driver() const
{
	return m_driver;
}

string UDevEvent::toString() const
{
	return Poco::format("%s,%s,%s,%s,%s",
		subsystem(), node(), type(), name(), driver());
}

AutoPtr<UDevEvent::Properties> UDevEvent::properties() const
{
	return m_properties;
}
