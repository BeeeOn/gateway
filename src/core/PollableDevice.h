#pragma once

#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>

#include "model/DeviceID.h"
#include "model/RefreshTime.h"
#include "core/Distributor.h"

namespace BeeeOn {

/**
 * @brief PollableDevice is a device that is necessary to
 * poll regularly for data. The polling can take some time
 * to progress and the time should be significantly smaller
 * than its refresh time.
 */
class PollableDevice {
public:
	typedef Poco::SharedPtr<PollableDevice> Ptr;

	virtual ~PollableDevice();

	/**
	 * @returns ID of the device.
	 */
	virtual DeviceID id() const = 0;

	/**
	 * @brief Regular period telling how often to call the method
	 * PollableDevice::poll(). The refresh must contain a valid
	 * time.
	 */
	virtual RefreshTime refresh() const = 0;

	/**
	 * @brief Perform polling for data and ship them via the
	 * given distributor.
	 */
	virtual void poll(Distributor::Ptr distributor) = 0;
};

}
