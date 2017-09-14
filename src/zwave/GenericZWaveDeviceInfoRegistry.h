#pragma once

#include <map>

#include <Poco/SharedPtr.h>

#include "zwave/VendorZWaveDeviceInfoRegistry.h"
#include "zwave/ZWaveDeviceInfoRegistry.h"
#include "util/Loggable.h"

namespace BeeeOn {

class GenericZWaveDeviceInfoRegistry:
	public ZWaveDeviceInfoRegistry,
	public Loggable {
public:
	ZWaveDeviceInfo::Ptr find(
		uint32_t vendor, uint32_t product) override;

	/**
	 * Register Z-Wave vendor to factory using vendor id.
	 * @param factory vendor object
	 */
	void registerVendor(
		VendorZWaveDeviceInfoRegistry::Ptr factory);

	void registerDefault(ZWaveDeviceInfoRegistry::Ptr factory);

private:
	std::map<uint32_t, VendorZWaveDeviceInfoRegistry::Ptr> m_vendors;
	ZWaveDeviceInfoRegistry::Ptr m_defaultVendor;
};

}
