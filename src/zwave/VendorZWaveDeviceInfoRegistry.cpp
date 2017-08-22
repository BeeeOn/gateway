#include <Poco/NumberParser.h>

#include "zwave/VendorZWaveDeviceInfoRegistry.h"

using namespace BeeeOn;
using namespace Poco;

VendorZWaveDeviceInfoRegistry::VendorZWaveDeviceInfoRegistry(
		uint32_t vendor):
	m_vendor(vendor)
{
}

uint32_t VendorZWaveDeviceInfoRegistry::vendor() const
{
	return m_vendor;
}

ZWaveDeviceInfo::Ptr VendorZWaveDeviceInfoRegistry::find(
	uint32_t vendor, uint32_t product)
{
	if (vendor == m_vendor)
		return findByProduct(product);

	throw InvalidArgumentException(
		"invalid vendor: " + std::to_string(m_vendor));
}
