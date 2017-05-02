#pragma once

#include <Poco/SharedPtr.h>

#include "zwave/ZWaveDeviceInfo.h"

namespace BeeeOn {

class ZWaveDeviceInfoRegistry {
public:
	typedef Poco::SharedPtr<ZWaveDeviceInfoRegistry> Ptr;

	virtual ~ZWaveDeviceInfoRegistry()
	{
	}

	/**
	 * It creates ZWaveDeviceInfo representing specific value implementation.
	 * @param vendor identification number
	 * @param product identification number
	 * @return ZWaveValueInfo specific for the given product
	 */
	virtual ZWaveDeviceInfo::Ptr find(
		uint32_t vendor, uint32_t product) = 0;
};

}
