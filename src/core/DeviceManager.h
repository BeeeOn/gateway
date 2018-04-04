#ifndef BEEEON_DEVICE_MANAGER_H
#define BEEEON_DEVICE_MANAGER_H

#include <set>
#include <typeindex>

#include <Poco/AtomicCounter.h>

#include "core/AnswerQueue.h"
#include "core/CommandHandler.h"
#include "core/CommandSender.h"
#include "core/DeviceCache.h"
#include "core/Distributor.h"
#include "loop/StoppableRunnable.h"
#include "model/DeviceID.h"
#include "model/DevicePrefix.h"
#include "model/ModuleID.h"
#include "util/Loggable.h"

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
	DeviceManager(const DevicePrefix &prefix,
		const std::initializer_list<std::type_index> &acceptable = {});
	virtual ~DeviceManager();

	/**
	 * @returns prefix managed by this device manager
	 */
	DevicePrefix prefix() const;

	/**
	* A generic stop implementation to be used by most DeviceManager
	* implementations. It just atomically sets the m_stop variable.
	*/
	void stop() override;

	void setDeviceCache(DeviceCache::Ptr cache);
	void setDistributor(Poco::SharedPtr<Distributor> distributor);

	/**
	 * Generic implementation of the CommandHandler::accept() method.
	 * If the m_acceptable set is initialized appropriately, this
	 * generic implementation can be used directly.
	 */
	bool accept(const Command::Ptr cmd) override;

	/**
	 * Generic implementation of the CommandHandler::handle() method.
	 * It works with respect to the accept() method and handles
	 * commands generically if possible. It also catches all exceptions
	 * and fails such command executions properly.
	 *
	 * To override handle, please use handleGeneric() instead.
	 */
	void handle(Command::Ptr cmd, Answer::Ptr answer) override;

protected:
	/**
	 * Generic implementation of the handle(). If any device manager
	 * needs to override the handle() method, it is more desirable to
	 * override handleGeneric().
	 */
	virtual void handleGeneric(const Command::Ptr cmd, Result::Ptr result);

	/**
	* Ship data received from a physical device into a collection point.
	*/
	void ship(const SensorData &sensorData);

	/**
	 * Obtain device list from server, method is blocking/non-blocking.
	 * Type of blocking is divided on the basis of timeout.
	 * Blocking waiting returns device list from server and non-blocking
	 * waiting returns device list from server or TimeoutException.
	 */
	std::set<DeviceID> deviceList(
		const Poco::Timespan &timeout = DEFAULT_REQUEST_TIMEOUT);

	/**
	 * Obtain Answer with Results which contains last measured value
	 * from server, method is blocking/non-blocking. Type of blocking
	 * is divided on the basis of timeout.
	 * Blocking waiting returns Answer with last measured value result
	 * from server and non-blocking waiting returns Answer with last
	 * measured value result from server or TimeoutException.
	 *
	 * If Answer contains several Results, the first Result SUCCESS will
	 * be selected.
	 */
	double lastValue(const DeviceID &deviceID, const ModuleID &moduleID,
		const Poco::Timespan &waitTime = DEFAULT_REQUEST_TIMEOUT);

	/**
	 * @returns the underlying DeviceCache instance
	 */
	DeviceCache::Ptr deviceCache() const;

private:
	void requestDeviceList(Answer::Ptr answer);
	std::set<DeviceID> responseDeviceList(
		const Poco::Timespan &waitTime, Answer::Ptr answer);

protected:
	static const Poco::Timespan DEFAULT_REQUEST_TIMEOUT;

	Poco::AtomicCounter m_stop;

private:
	DevicePrefix m_prefix;
	DeviceCache::Ptr m_deviceCache;
	Poco::SharedPtr<Distributor> m_distributor;
	std::set<std::type_index> m_acceptable;
};

}

#endif
