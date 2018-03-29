#include <string>

#include "di/Injectable.h"
#include "zwave/GenericZWaveDeviceInfoRegistry.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

BEEEON_OBJECT_BEGIN(BeeeOn, GenericZWaveDeviceInfoRegistry)
BEEEON_OBJECT_CASTABLE(ZWaveDeviceInfoRegistry)
BEEEON_OBJECT_PROPERTY("registers", &GenericZWaveDeviceInfoRegistry::registerVendor)
BEEEON_OBJECT_PROPERTY("defaultRegistry", &GenericZWaveDeviceInfoRegistry::registerDefault)
BEEEON_OBJECT_END(BeeeOn, GenericZWaveDeviceInfoRegistry)

ZWaveDeviceInfo::Ptr GenericZWaveDeviceInfoRegistry::find(
		uint32_t vendor, uint32_t product)
{
	auto search = m_vendors.find(vendor);
	if (search != m_vendors.end())
		return search->second->find(vendor, product);

	logger().debug(
		"vendor " + to_string(vendor)
		+ " is not registered",
		__FILE__, __LINE__);

	if (m_defaultVendor.isNull()) {
		throw IllegalStateException(
			"default ZWaveDeviceInfoRegistry is not registered");
	}

	return m_defaultVendor->find(vendor, product);
}

void GenericZWaveDeviceInfoRegistry::registerVendor(
	VendorZWaveDeviceInfoRegistry::Ptr factory)
{
	auto it = m_vendors.emplace(factory->vendor(), factory);

	if (!it.second) {
		throw ExistsException(
			"vendor "
			+ to_string(factory->vendor())
			+ " is already registered"
		);
	}
}

void GenericZWaveDeviceInfoRegistry::registerDefault(
	ZWaveDeviceInfoRegistry::Ptr factory)
{
	m_defaultVendor = factory;
}
