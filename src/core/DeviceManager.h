#ifndef BEEEON_DEVICE_MANAGER_H
#define BEEEON_DEVICE_MANAGER_H

#include <set>
#include <typeindex>

#include <Poco/AtomicCounter.h>
#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>

#include "commands/DeviceAcceptCommand.h"
#include "commands/DeviceUnpairCommand.h"
#include "commands/GatewayListenCommand.h"
#include "core/AnswerQueue.h"
#include "core/CommandHandler.h"
#include "core/CommandSender.h"
#include "core/DeviceCache.h"
#include "core/Distributor.h"
#include "loop/StoppableRunnable.h"
#include "loop/StopControl.h"
#include "model/DeviceID.h"
#include "model/DevicePrefix.h"
#include "model/ModuleID.h"
#include "util/AsyncWork.h"
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
	typedef Poco::SharedPtr<DeviceManager> Ptr;

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
	 * @brief Generic handler of the DeviceAcceptCommand. It might be helpful to
	 * override this method in case we need to make some technology-specific check
	 * of the device to be accepted. The default implementation simply marks the
	 * given device as paired.
	 */
	virtual void handleAccept(const DeviceAcceptCommand::Ptr);

	/**
	 * @brief Starts device discovery process in a technology-specific way.
	 * This method is always called inside a critical section and so
	 * its implementation does not have to be thread-safe nor reentrant
	 * (unless it cooperates with other threads itself).
	 *
	 * The purpose of this call is to initialize and start the discovery
	 * process which might be a non-blocking operation. The caller uses
	 * the provided AsyncWork<> instance to wait until it finishes or to
	 * cancel it earlier if needed.
	 */
	virtual AsyncWork<>::Ptr startDiscovery(const Poco::Timespan &timeout);

	/**
	 * @brief Implements handling of the listen command in a generic way. The method
	 * ensures that only 1 thread can execute the discovery process at a time.
	 *
	 * It uses the method startDiscovery() to initialize and start a new discovery
	 * process. It is guaranteed that the startDiscovery() method is always called
	 * exactly once at a time and until it finishes, no other discovery is started.
	 * The minimal timeout for the discovery is at least 1 second which is enforced
	 * by the implementation.
	 */
	void handleListen(const GatewayListenCommand::Ptr cmd);

	/**
	 * @brief Starts device unpair process in a technology-specific way.
	 * This method is always called inside a critical section and so its
	 * implementation does not have to be thread-safe nor reentrant
	 * (unless it cooperates with other threads itself).
	 *
	 * The unpairing process might be a non-blocking operation. The unpaired
	 * devices are provided via the result of the returned AsyncWork instance.
	 */
	virtual AsyncWork<std::set<DeviceID>>::Ptr startUnpair(
			const DeviceID &id,
			const Poco::Timespan &timeout);

	/**
	 * @brief Implements handling of the unpair command in a generic way. The method
	 * ensures that only 1 thread can execute the unpair process at a time.
	 *
	 * It uses the method startUnpair() to initialize and start the unpairing process.
	 * The method startUnpair() is always called exactly once at a time and until it
	 * finishes, no other unpair is started.
	 */
	std::set<DeviceID> handleUnpair(const DeviceUnpairCommand::Ptr cmd);

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
	StopControl m_stopControl;

private:
	DevicePrefix m_prefix;
	DeviceCache::Ptr m_deviceCache;
	Poco::FastMutex m_listenLock;
	Poco::FastMutex m_unpairLock;
	Poco::SharedPtr<Distributor> m_distributor;
	std::set<std::type_index> m_acceptable;
};

}

#endif
