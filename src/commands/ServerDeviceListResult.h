#ifndef BEEEON_SERVER_DEVICE_LIST_RESULT_H
#define BEEEON_SERVER_DEVICE_LIST_RESULT_H

#include <vector>

#include <Poco/Mutex.h>

#include "core/Answer.h"
#include "core/Result.h"
#include "model/DeviceID.h"

namespace BeeeOn {

/*
 * The result for ServerDeviceListCommand that includes
 * device list for the particular Prefix.
 */
class ServerDeviceListResult : public Result {
public:
	typedef Poco::AutoPtr<ServerDeviceListResult> Ptr;

	ServerDeviceListResult(const Answer::Ptr answer);

	void setDeviceList(const std::vector<DeviceID> &deviceList);
	void setDeviceListUnlocked(const std::vector<DeviceID> &deviceList);

	std::vector<DeviceID> deviceList() const;
	std::vector<DeviceID> deviceListUnlocked() const;

protected:
	~ServerDeviceListResult();

private:
	std::vector<DeviceID> m_deviceList;
};

}

#endif
