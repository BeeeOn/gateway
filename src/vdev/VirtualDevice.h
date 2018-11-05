#pragma once

#include <list>

#include <Poco/Event.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>

#include "core/Result.h"
#include "model/DeviceID.h"
#include "model/ModuleID.h"
#include "model/ModuleType.h"
#include "model/RefreshTime.h"
#include "model/SensorData.h"
#include "vdev/VirtualModule.h"

namespace BeeeOn {

class VirtualDevice {
public:
	typedef Poco::SharedPtr<VirtualDevice> Ptr;

	VirtualDevice();
	~VirtualDevice();

	void setDeviceId(const DeviceID &deviceId);
	DeviceID deviceID() const;

	void setVendorName(const std::string &vendorName);
	std::string vendorName() const;

	void setProductName(const std::string &productName);
	std::string productName() const;

	std::list<VirtualModule::Ptr> modules() const;
	void addModule(const VirtualModule::Ptr virtualModule);
	std::list<ModuleType> moduleTypes() const;

	void setRefresh(const RefreshTime &refresh);
	RefreshTime refresh() const;

	bool modifyValue(
		const ModuleID &moduleID, double value);
	SensorData generate();

private:
	RefreshTime m_refresh;
	std::string m_vendorName;
	std::string m_productName;
	std::list<VirtualModule::Ptr> m_modules;
	DeviceID m_deviceID;
};

}
