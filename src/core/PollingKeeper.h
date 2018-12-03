#pragma once

#include <map>

#include "core/DevicePoller.h"
#include "core/PollableDevice.h"

namespace BeeeOn {

/**
 * @brief PollingKeeper takes care of devices that are being polled.
 * It cancels all polled devices it manages upon request or destruction
 * to avoid leaking unstopped polled devices. The class is NOT thread-safe.
 */
class PollingKeeper {
public:
	PollingKeeper();

	/**
	 * @brief Cancel all registered pollable devices.
	 */
	~PollingKeeper();

	/**
	 * @brief Configure DevicePoller to use.
	 */
	void setDevicePoller(DevicePoller::Ptr poller);

	/**
	 * @brief Register the given device and schedule it into
	 * the underlying poller.
	 */
	void schedule(PollableDevice::Ptr device);

	/**
	 * @brief Cancel polling of the device represented by the
	 * given ID and unregister it.
	 */
	void cancel(const DeviceID &id);

	/**
	 * @brief Cancel all registered pollable devices.
	 */
	void cancelAll();

private:
	std::map<DeviceID, PollableDevice::Ptr> m_polled;
	DevicePoller::Ptr m_devicePoller;
};

}
