#pragma once

#include <Poco/Clock.h>
#include <Poco/Mutex.h>

#include "core/DeviceCache.h"
#include "core/Distributor.h"
#include "core/PollableDevice.h"
#include "loop/StoppableRunnable.h"
#include "loop/StopControl.h"
#include "util/AsyncExecutor.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief DevicePoller is a scheduler for PollableDevice instances.
 * Any number of devices can be scheduled for regular polling of
 * their state. Each device can be scheduled according to its
 * refresh time and later cancelled from being polled.
 */
class DevicePoller : public StoppableRunnable, Loggable {
public:
	typedef Poco::SharedPtr<DevicePoller> Ptr;

	DevicePoller();

	void setDistributor(Distributor::Ptr distributor);
	void setPollExecutor(AsyncExecutor::Ptr executor);

	/**
	 * @brief Configure time threshold that would enable to fire
	 * a warning about a too slow device. Polling of device should
	 * take longer (or much longer) than its refresh time.
	 *
	 * The threshold can also be negative which allows to report
	 * devices that poll() almost as long as their refresh time.
	 */
	void setWarnThreshold(const Poco::Timespan &threshold);

	/**
	 * @brief Schedule the given device relatively to the given
	 * time reference (usually meaning now). An already scheduled
	 * device is not rescheduled or updated in anyway.
	 */
	void schedule(
		PollableDevice::Ptr device,
		const Poco::Clock &now = {});

	/**
	 * @brief Cancel the device of the given ID if exists.
	 * If the device is currently being polled, it would not
	 * be interrupted but its will not be rescheduled.
	 */
	void cancel(const DeviceID &id);

	/**
	 * @brief Poll devices according to the schedule.
	 */
	void run() override;

	/**
	 * @brief Stop polling of devices.
	 */
	void stop() override;

	/**
	 * @brief Remove all scheduled devices (kind of reset).
	 */
	void cleanup();

protected:
	/**
	 * @brief Get refresh time of the device and check whether it
	 * is usable for regular polling. If it is not, throw an exception.
	 * @return refresh time (positive value)
	 * @throws Poco::IllegalStateException when the refresh time is not usable
	 */
	static Poco::Timespan grabRefresh(const PollableDevice::Ptr device);

	/**
	 * @brief Reschedule device after its PollableDevice::poll() method
	 * has been called. Only active devices are rescheduled.
	 */
	void reschedule(
		PollableDevice::Ptr device,
		const Poco::Clock &now = {});

	/**
	 * @brief Do the actual scheduling - registration into m_devices,
	 * and m_schedule containers. If the devices is already scheduled,
	 * its previous record is just updated. Not thread-safe.
	 */
	void doSchedule(
		PollableDevice::Ptr device,
		const Poco::Clock &now = {});

	/**
	 * @brief Check the next device to be polled. If the next device
	 * is scheduled into the future, return the time difference.
	 * Otherwise return 0 as the device is currently polled.
	 */
	Poco::Timespan pollNextIfOnSchedule(const Poco::Clock &now = {});

	/**
	 * @brief Invoke the PollableDevice::poll() method via the configured
	 * m_pollExecutor. Thus, the poll() is usually called asynchronously
	 * and it can be parallelized with other devices.
	 */
	void doPoll(PollableDevice::Ptr device);

private:
	Distributor::Ptr m_distributor;
	AsyncExecutor::Ptr m_pollExecutor;
	Poco::Timespan m_warnThreshold;

	typedef std::multimap<Poco::Clock, PollableDevice::Ptr> Schedule;
	Schedule m_schedule;
	std::map<DeviceID, Schedule::const_iterator> m_devices;
	std::set<DeviceID> m_active;
	Poco::FastMutex m_lock;

	StopControl m_stopControl;
};

}
