#ifndef BEEEON_DONGLE_DEVICE_MANAGER_H
#define BEEEON_DONGLE_DEVICE_MANAGER_H

#include <Poco/Event.h>
#include <Poco/Mutex.h>

#include "core/DeviceManager.h"
#include "hotplug/HotplugListener.h"

namespace BeeeOn {

class DongleDeviceManager : public DeviceManager, public HotplugListener {
public:
	DongleDeviceManager(const DevicePrefix &prefix);

	void run() override;
	void stop() override;

	void onAdd(const HotplugEvent &e) override;
	void onRemove(const HotplugEvent &e) override;

protected:
	/**
	 * If the event represents an appropriate dongle, it should
	 * return its name or path that can be used for its access.
	 *
	 * If the event represents an inappropriate device, it returns
	 * an empty string.
	 */
	virtual std::string dongleMatch(const HotplugEvent &e) = 0;

	/**
	 * The main execution loop that is to be run while the appropriate
	 * dongle is available. When the dongle is disconnected during
	 * the execution, the method must throw an exception
	 * (a good choice is Poco::IOException).
	 *
	 * When the method returns normally, the DongleDeviceManager finishes
	 * its main loop and exits the thread.
	 */
	virtual void dongleAvailable() = 0;

	/**
	 * Called when no appropriate dongle is available for this
	 * device manager. The implementation can wait by using the
	 * event() that is signalled when the dongle is available again.
	 * In such case, the method should return false on wakeup.
	 * Otherwise, return true to use a built-in waiting for event().
	 *
	 * The default implementation just returns true.
	 */
	virtual bool dongleMissing();

	/**
	 * Return the name of associated dongle.
	 * If the failWhenMissing is true, then it throws an exception
	 * when no such dongle name is available (disconnected).
	 */
	std::string dongleName(bool failWhenMissing = true) const;

	/**
	 * A general-purpose event. It is signalized when the dongle
	 * becomes available or unavailable. It is used by the
	 * DongleDeviceManager for waiting purposes.
	 */
	Poco::Event &event();

private:
	mutable Poco::FastMutex m_lock;
	Poco::Event m_event;
	std::string m_dongleName;
};

}

#endif
