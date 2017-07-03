#ifndef BEEEON_DEVICE_MANAGER_H
#define BEEEON_DEVICE_MANAGER_H

#include <Poco/AtomicCounter.h>

#include "core/AnswerQueue.h"
#include "core/CommandHandler.h"
#include "core/CommandSender.h"
#include "core/Distributor.h"
#include "loop/StoppableRunnable.h"

namespace BeeeOn {

/**
 * All classes that manage devices should inherit from this
 * abstract class. It provides a common functionality for this
 * purpose.
 *
 * There is usually a main thread that performs communication
 * with the physical devices and translates the specific device
 * communication protocol into the Command & Answer interface
 * or into SensorData interface.
 *
 * Communication in the direction to physical devices is served
 * via the CommandHandler interface. By accepting Commands asking
 * for specific tasks, the physical devices can be queried as
 * expected by the server.
 */
class DeviceManager:
	public CommandHandler,
	public CommandSender,
	protected Loggable,
	public StoppableRunnable {
public:
	DeviceManager(const std::string &name);
	virtual ~DeviceManager();

	/**
	* A generic stop implementation to be used by most DeviceManager
	* implementations. It just atomically sets the m_stop variable.
	*/
	void stop() override;

	void setDistributor(Poco::SharedPtr<Distributor> distributor);

protected:
	/**
	* Ship data received from a physical device into a collection point.
	*/
	void ship(const SensorData &sensorData);

protected:
	Poco::AtomicCounter m_stop;

private:
	Poco::SharedPtr<Distributor> m_distributor;
};

}

#endif
