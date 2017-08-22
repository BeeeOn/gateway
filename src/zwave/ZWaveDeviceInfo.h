#pragma once

#include <string>
#include <vector>

#include <Poco/AutoPtr.h>
#include <Poco/RefCountedObject.h>

#include <openzwave/Manager.h>

#include "model/ModuleType.h"
#include "zwave/ZWaveNodeInfo.h"
#include "zwave/ZWaveClassRegistry.h"

namespace BeeeOn {

class ZWaveDeviceInfo : public Poco::RefCountedObject {
public:
	typedef Poco::AutoPtr<ZWaveDeviceInfo> Ptr;

	virtual ~ZWaveDeviceInfo();

	/**
	 * Extract data from ValueID and parse to double.
	 */
	virtual double extractValue(const OpenZWave::ValueID &valueID,
		const ModuleType &moduleType);

	/**
	 * Method converts inserted value from server to value that is known
	 * by OpenZWave library when it sets value.
	 */
	virtual std::string convertValue(double value);

	virtual Poco::Timespan refreshTime() const;

	void setRegistry(ZWaveClassRegistry::Ptr registry);
	ZWaveClassRegistry::Ptr registry() const;

private:
	ZWaveClassRegistry::Ptr m_registry;
};

}
