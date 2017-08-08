#ifndef BEEEON_BLUETOOTH_AVAILABILITY_MANAGER_H
#define BEEEON_BLUETOOTH_AVAILABILITY_MANAGER_H

#include <map>
#include <string>

#include <Poco/Event.h>
#include <Poco/Logger.h>
#include <Poco/Mutex.h>
#include <Poco/RunnableAdapter.h>
#include <Poco/Thread.h>
#include <Poco/Timespan.h>

#include "bluetooth/BluetoothDevice.h"
#include "bluetooth/HciInterface.h"
#include "core/Answer.h"
#include "core/DongleDeviceManager.h"
#include "model/DeviceID.h"
#include "model/SensorData.h"

namespace BeeeOn {

class BluetoothAvailabilityManager : public DongleDeviceManager {
public:
	BluetoothAvailabilityManager();

	/**
	 * Main thread.
	 * @throw when is any problem with scan
	 */
	void dongleAvailable() override;

	bool dongleMissing() override;

	std::string dongleMatch(const UDevEvent &e) override;

	void stop() override;

	Poco::Timespan detectAll(const HciInterface &hci);

	/**
	 * How often will be scan of paired devices
	 */
	void setWakeUpTime(int time);

	bool accept(const Command::Ptr cmd) override;

	void handle(Command::Ptr cmd, Answer::Ptr answer) override;

	void doDeviceAcceptCommand(
		const Command::Ptr &cmd, const Answer::Ptr &answer);

	void doUnpairCommand(
		const Command::Ptr &cmd, const Answer::Ptr &answer);

	void doListenCommand(const Command::Ptr &cmd, const Answer::Ptr &answer);

private:
	bool haveTimeForInactive(Poco::Timespan elapsedTime);

	void fetchDeviceList();

	void listen();

	void addDevice(const DeviceID &id);

	bool hasDevice(const DeviceID &id);

	void removeDevice(const DeviceID &id);

	void shipStatusOf(const BluetoothDevice &device);

	void sendNewDevice(const DeviceID &id, const std::string &name);

	std::list<ModuleType> moduleTypes() const;

	DeviceID createDeviceID(const MACAddress &numMac) const;

	Poco::RunnableAdapter<BluetoothAvailabilityManager> m_listenThread;
	Poco::Timespan m_wakeUpTime;
	Poco::Thread m_thread;
	std::map<DeviceID, BluetoothDevice> m_deviceList;
	Poco::Event m_stopEvent;
	Poco::FastMutex m_lock;
};

}

#endif //BEEEON_BLUETOOTH_AVAILABILITY_MANAGER_H
