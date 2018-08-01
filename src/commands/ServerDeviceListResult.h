#ifndef BEEEON_SERVER_DEVICE_LIST_RESULT_H
#define BEEEON_SERVER_DEVICE_LIST_RESULT_H

#include <map>
#include <vector>

#include <Poco/Mutex.h>
#include <Poco/Nullable.h>

#include "core/Answer.h"
#include "core/Result.h"
#include "model/DeviceID.h"
#include "model/ModuleID.h"

namespace BeeeOn {

/*
 * The result for ServerDeviceListCommand that includes
 * device list for the particular Prefix.
 */
class ServerDeviceListResult : public Result {
public:
	typedef Poco::AutoPtr<ServerDeviceListResult> Ptr;
	typedef std::map<ModuleID, double> ModuleValues;
	typedef std::map<DeviceID, ModuleValues> DeviceValues;

	ServerDeviceListResult(const Answer::Ptr answer);

	void setDevices(const DeviceValues &values);
	DeviceValues devices() const;

	void setDeviceList(const std::vector<DeviceID> &deviceList);
	std::vector<DeviceID> deviceList() const;

	Poco::Nullable<double> value(const DeviceID &id, const ModuleID &module) const;

protected:
	~ServerDeviceListResult();

private:
	DeviceValues m_data;
};

}

#endif
