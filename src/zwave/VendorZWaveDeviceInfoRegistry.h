#pragma once

#include <Poco/SharedPtr.h>

#include "zwave/ZWaveDeviceInfoRegistry.h"

namespace BeeeOn {

class VendorZWaveDeviceInfoRegistry : public ZWaveDeviceInfoRegistry {
public:
	typedef Poco::SharedPtr<VendorZWaveDeviceInfoRegistry> Ptr;

	VendorZWaveDeviceInfoRegistry(uint32_t vendor);

	uint32_t vendor() const;

	ZWaveDeviceInfo::Ptr find(
		uint32_t vendor, uint32_t product) override;

protected:
	virtual ZWaveDeviceInfo::Ptr findByProduct(uint32_t product) = 0;

private:
	uint32_t m_vendor;
};

}
