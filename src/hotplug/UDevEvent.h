#ifndef BEEEON_UDEV_EVENT_H
#define BEEEON_UDEV_EVENT_H

#include <string>

#include <Poco/AutoPtr.h>
#include <Poco/Util/PropertyFileConfiguration.h>

namespace BeeeOn {

class UDevEvent {
public:
	typedef Poco::Util::PropertyFileConfiguration Properties;

	UDevEvent();

	void setSubsystem(const std::string &subsystem);
	std::string subsystem() const;

	void setNode(const std::string &node);
	std::string node() const;

	void setType(const std::string &type);
	std::string type() const;

	void setName(const std::string &name);
	std::string name() const;

	void setDriver(const std::string &driver);
	std::string driver() const;

	std::string toString() const;

	Poco::AutoPtr<Properties> properties() const;

private:
	std::string m_node;
	std::string m_subsystem;
	std::string m_type;
	std::string m_name;
	std::string m_driver;
	Poco::AutoPtr<Properties> m_properties;
};

}

#endif
