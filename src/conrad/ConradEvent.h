#pragma once

#include <string>

#include <Poco/Exception.h>
#include <Poco/JSON/Object.h>
#include <Poco/SharedPtr.h>

#include "model/DeviceID.h"
#include "util/JsonUtil.h"

namespace BeeeOn {

/**
 * Stores information about Conrad messages.
 */
class ConradEvent{
public:
	typedef Poco::SharedPtr<ConradEvent> Ptr;

protected:
	ConradEvent();

public:
	static ConradEvent parse(
		DeviceID const &deviceID,
		const Poco::JSON::Object::Ptr message);

	DeviceID id() const;
	double rssi() const;
	std::string event() const;
	std::string raw() const;
	std::string type() const;
	std::string channels() const;
	std::string protState() const;

private:
	DeviceID m_id;
	double m_rssi;
	std::string m_event;
	std::string m_raw;
	std::string m_type;
	std::string m_channels;
	std::string m_protState;
};

}
