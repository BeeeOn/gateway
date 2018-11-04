#pragma once

#include <string>

#include "hotplug/HotplugEvent.h"

namespace BeeeOn {

class HciUtil {
public:
	/**
	 * @brief If the given hotplug event represents a bluetooth
	 * controller device, it returns its name. Otherwise, it returns
	 * an empty string.
	 */
	static std::string hotplugMatch(const HotplugEvent &event);
};

}
