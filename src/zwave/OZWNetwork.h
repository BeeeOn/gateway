#pragma once

#include <list>
#include <map>
#include <set>
#include <string>

#include <Poco/AtomicCounter.h>
#include <Poco/Mutex.h>
#include <Poco/Path.h>
#include <Poco/Timespan.h>

#include "hotplug/HotplugListener.h"
#include "loop/StoppableLoop.h"
#include "util/EventSource.h"
#include "util/PeriodicRunner.h"
#include "zwave/AbstractZWaveNetwork.h"
#include "zwave/OZWCommand.h"
#include "zwave/ZWaveListener.h"
#include "zwave/ZWaveNode.h"

namespace OpenZWave {

class Notification;
class ValueID;

}

namespace BeeeOn {

/**
 * @brief OZWNetwork manages the Z-Wave network by using the OpenZWave library (OZW).
 * Its purpose is to handle OZW notifications and initiate OZW commands if needed.
 *
 * The OZW library has multiple configuration options. Some are set internally to
 * some sane values (unimportant for the BeeeOn gateway), others can be changed by
 * OZWNetwork properties.
 *
 * To initialize, the OZWNetwork::configure() is to be used. The method is called
 * automatically by DI. The deinitialization is implemented via OZWNetwork::cleanup()
 * (also called by DI).
 *
 * The OZWNetwork is able to handle multiple Z-Wave dongles (according to OZW). It
 * assigns dongles via the hotplug mechanism. It recognizes dongles with property
 * tty.BEEEON_DONGLE == "zwave". Currently, only dongles connected via tty are supported.
 *
 * Everytime when a Z-Wave dongle is detected via OZWNetwork::onAdd(), the OZW library
 * is notified and starts a thread for the driver. Drivers are removed on hot-unplug
 * via OZWNetwork::onRemove() or when the OZWNetwork::cleanup() is called.
 *
 * The OZWNetwork utilizes AsyncExecutor for performing asynchronous tasks that must
 * not be performed from the OZW notification handler function.
 */
class OZWNetwork :
	public HotplugListener,
	public AbstractZWaveNetwork {
	friend class OZWCommand;
public:
	OZWNetwork();
	~OZWNetwork();

	/**
	 * @brief Set OZW configPath (contains definitions, XML files, etc.).
	 * The directory should exist prior to calling OZWNetwork::configure().
	 *
	 * @see OZWNetwork::checkDirectory()
	 */
	void setConfigPath(const std::string &path);

	/**
	 * @brief Set OZW userPath (cache of device definitions). This directory
	 * would be created if it does not exist.
	 *
	 * @see OZWNetwork::prepareDirectory()
	 */
	void setUserPath(const std::string &path);

	/**
	 * @brief Set OZW PollInterval option.
	 */
	void setPollInterval(const Poco::Timespan &interval);

	/**
	 * @brief Set OZW IntervalBetweenPolls option.
	 */
	void setIntervalBetweenPolls(bool enable);

	/**
	 * @brief Set OZW RetryTimeout option.
	 */
	void setRetryTimeout(const Poco::Timespan &timeout);

	/**
	 * @brief Set OZW AssumeAwake option.
	 */
	void setAssumeAwake(bool awake);

	/**
	 * @brief Set OZW DriverMaxAttempts option.
	 */
	void setDriverMaxAttempts(int attempts);

	/**
	 * @brief Set OZW NetworkKey option. The key is expected to
	 * be 16 bytes long.
	 */
	void setNetworkKey(const std::list<std::string> &bytes);

	/**
	 * @brief Set the interval of reporting OZW statistics.
	 */
	void setStatisticsInterval(const Poco::Timespan &interval);

	/**
	 * @brief Set controllers (list of home IDs) to be reset
	 * upon their first appearance in the network.
	 */
	void setControllersToReset(const std::list<std::string> &homes);

	/**
	 * @brief Set asynchronous executor used for asynchronous tasks
	 * and events reporting.
	 */
	void setExecutor(AsyncExecutor::Ptr executor);

	/**
	 * @brief Register a ZWaveListener that would be receiving events.
	 */
	void registerListener(ZWaveListener::Ptr listener);

	/**
	 * @brief Handle incoming OZW notifications in the context
	 * of the OZWNetwork instance.
	 */
	void onNotification(const OpenZWave::Notification *n);

	/**
	 * @brief If the event represents a compatible Z-Wave dongle,
	 * an appropriate driver is added into the OZW runtime.
	 */
	void onAdd(const HotplugEvent &event) override;

	/**
	 * @brief If the event represents a compatible Z-Wave dongle,
	 * the appropriate driver is removed from the OZW runtime.
	 */
	void onRemove(const HotplugEvent &event) override;

	/**
	 * @brief Initialize OZW library, set options and register self
	 * as a watcher for handling notifications. The statistics reporter
	 * is started.
	 */
	void configure();

	/**
	 * @brief Deinitialize OZW library. Stop the statistics reporter.
	 */
	void cleanup();

protected:
	/**
	 * @brief OZWNode wraps the ZWaveNode to be able to hold specific
	 * data related to the OpenZWave library.
	 */
	class OZWNode : public ZWaveNode {
	public:
		OZWNode(const Identity &id, bool controller = false);

		/**
		 * @brief Register the command class together with ValueID
		 * representation as provided by the OpenZWave library.
		 * Calls ZWaveNode::add() internally.
		 */
		void add(const CommandClass &cc, const OpenZWave::ValueID &id);

		/**
		 * @brief Return the appropriate ValueID for the given command class.
		 * @throws Poco::NotFoundException
		 */
		OpenZWave::ValueID operator[](const CommandClass &cc) const;

	private:
		std::map<CommandClass, OpenZWave::ValueID> m_valueIDs;
	};

	/**
	 * @brief Check that the given directory exists and is readable.
	 * @throws FileAccessDeniedException if the directory is not readable
	 */
	void checkDirectory(const Poco::Path &path);

	/**
	 * @brief Create the directory represented by the given path.
	 * If it already exists, it must be writable and readable.
	 */
	void prepareDirectory(const Poco::Path &path);

	/**
	 * @brief Fire Z-Wave statistics. This is called periodically by the
	 * m_statisticsRunner.
	 *
	 * @see OZWNetwork::setExecutor()
	 * @see OZWNetwork::registerListener()
	 * @see OZWNetwork::setStatisticsInterval()
	 */
	void fireStatistics();

	/**
	 * @brief Determine hotplugged devices compatible with the OZWNetwork.
	 * The property tty.BEEEON_DONGLE is tested to equal to "zwave".
	 */
	static bool matchEvent(const HotplugEvent &event);

	/**
	 * The OpenZWave library uses a notification loop to provide information
	 * about the Z-Wave network. A notification represents e.g. detection of a
	 * new device, change of a value, Z-Wave dongle initialization, etc.
	 */
	static void ozwNotification(
		OpenZWave::Notification const *n,
		void *context);

	/**
	 * @brief Certain notifications coming from OZW are to be ignored because
	 * they are uninteresting or known to screw up certain things. This call
	 * filters all such notifications.
	 */
	bool ignoreNotification(const OpenZWave::Notification *n) const;

	/**
	 * @brief Initiate asynchronous reset of controller associated
	 * with the given home ID.
	 *
	 * The controller reset is invoked via the m_executor instance.
	 * During the reset procedure, the driverRemoved() would be called
	 * for the given home ID.
	 */
	void resetController(const uint32_t home);

	/**
	 * @brief Called when the OZW driver becomes ready to work
	 * for the given home ID. The OZWNetwork installs the home
	 * ID and if configured, it performs reset of the associated
	 * controller.
	 */
	void driverReady(const OpenZWave::Notification *n);

	/**
	 * @brief Called when OZW driver fails to become ready.
	 * The associated home ID is uninstalled.
	 */
	void driverFailed(const OpenZWave::Notification *n);

	/**
	 * @brief Called when OZW driver is removed from the system.
	 *
	 * This happens usually when a Z-Wave dongle is removed or
	 * its controller is being reset.
	 */
	void driverRemoved(const OpenZWave::Notification *n);

	/**
	 * @brief Find out whether the given node ID represents a controller
	 * of the given home.
	 */
	bool checkNodeIsController(const uint32_t home, const uint8_t node) const;

	/**
	 * @brief Called when the OZW discovered a new Z-Wave node.
	 */
	void nodeNew(const OpenZWave::Notification *n);

	/**
	 * @brief Called when the OZW added a Z-Wave node to its list.
	 *
	 * An instance of ZWaveNode is maintained for every such node.
	 * At this stage we might have not enough information about
	 * that node.
	 */
	void nodeAdded(const OpenZWave::Notification *n);

	/**
	 * @brief Called when OZW resolves more details about a Z-Wave node.
	 *
	 * At this stage, we know identification details about the Z-Wave
	 * node. Such information are maintained in the associated ZWaveNode
	 * instance.
	 *
	 * @see ZWaveNode::setProductId()
	 * @see ZWaveNode::setProduct()
	 * @see ZWaveNode::setProductType()
	 * @see ZWaveNode::setVendorId()
	 * @see ZWaveNode::setVendor()
	 */
	void nodeNaming(const OpenZWave::Notification *n);

	/**
	 * @brief Called when OZW resolves properties of a Z-Wave node.
	 *
	 * After this call, the associated ZWaveNode instance would have
	 * information about the features supported by that Z-Wave node.
	 *
	 * @see ZWaveNode::setSupport()
	 */
	void nodeProtocolInfo(const OpenZWave::Notification *n);

	/**
	 * @brief Called when OZW believes that the given Z-Wave node
	 * is ready for standard operations.
	 *
	 * The event EVENT_NEW_NODE will be delivered to upper layers having
	 * information about a new available node for further work.
	 *
	 * Note that, the Z-Wave node might not be discovered fully at this
	 * stage. However, ig the node is already known by the upper layers,
	 * the node can be directly used without waiting for more details
	 * which might take quite a long time.
	 *
	 * @see ZWaveNetwork::pollEvent()
	 * @see PollEvent::EVENT_NEW_NODE
	 */
	void nodeReady(const OpenZWave::Notification *n);

	/**
	 * @brief Called when OZW assumes that a Z-Wave node was removed
	 * from the network and thus is unreachable.
	 *
	 * The event EVENT_REMOVE_NODE will be delivered to upper layers
	 * with information the node removal.
	 *
	 * The OZW persistent configuration cache is written here.
	 *
	 * @see ZWaveNetwork::pollEvent()
	 * @see PollEvent::EVENT_REMOVE_NODE
	 */
	void nodeRemoved(const OpenZWave::Notification *n);

	/**
	 * @brief Called when OZW discoveres a new value associated with a
	 * certain Z-Wave node.
	 *
	 * The value is checked and a proper CommandClass
	 * instance created and added to the appropriate ZWaveNode instance.
	 *
	 * @see buildCommandClass()
	 */
	void valueAdded(const OpenZWave::Notification *n);

	/**
	 * @brief Called when OZW received data associated with a Z-Wave
	 * node's value. Such data are processed by the upper layers.
	 *
	 * The event EVENT_VALUE will be delivered to upper layers having
	 * the new data.
	 *
	 * @see ZWaveNetwork::pollEvent()
	 * @see PollEvent::EVENT_VALUE
	 */
	void valueChanged(const OpenZWave::Notification *n);

	/**
	 * @brief Called when OZW finishes discovering of a Z-Wave node.
	 * At this moment, we are sure to know all dynamic information about
	 * the node (including values definitions).
	 *
	 * The event EVENT_UPDATE_NODE will be delivered to upper layers having
	 * updated information about a node. The associated ZWaveNode instance
	 * is marked to be queried by this call.
	 *
	 * The OZW persistent configuration cache is written here.
	 *
	 * @see ZWaveNode::setQueried()
	 * @see ZWaveNetwork::pollEvent()
	 * @see PollEvent::EVENT_UPDATE_NODE
	 */
	void nodeQueried(const OpenZWave::Notification *n);

	/**
	 * @brief Called when OZW believes that all awaken node have been queried
	 * fully. Here we analyze some stats about nodes (queried, failed, total,
	 * etc.).
	 *
	 * All nodes of the associated home ID are checked whether they are awake.
	 * The awake nodes marked as not-queried are updated and the event
	 * EVENT_UPDATE_NODE is emitted to upper layers.
	 *
	 * Finally, the EVENT_READY will be delivered to upper layers.
	 *
	 * @see ZWaveNode::setQueried()
	 * @see ZWaveNetwork::pollEvent()
	 * @see PollEvent::EVENT_UPDATE_NODE
	 * @see PollEvent::EVENT_READY
	 */
	void awakeNodesQueried(const OpenZWave::Notification *n);

	/**
	 * @brief Called when OZW believes that all nodes have been queried fully.
	 * This call is similar to awakeNodesQueried(). We perform some statistics
	 * collection and update certain nodes if needed.
	 *
	 * All nodes marked as non-queried that are not failing are updated and
	 * the event EVENT_UPDATE_NODE is emitted to upper layers.
	 *
	 * Finally, the EVENT_READY will be delivered to upper layers.
	 *
	 * @see ZWaveNode::setQueried()
	 * @see ZWaveNetwork::pollEvent()
	 * @see PollEvent::EVENT_UPDATE_NODE
	 * @see PollEvent::EVENT_READY
	 */
	void allNodesQueried(const OpenZWave::Notification *n);

	/**
	 * @brief Helper method to print the home ID.
	 */
	static std::string homeAsString(uint32_t home);

	/**
	 * @brief Helper method to build a CommandClass instance directly
	 * from the OZW ValueID instance.
	 */
	static ZWaveNode::CommandClass buildCommandClass(
			const OpenZWave::ValueID &id);

	/**
	 * @brief Start the inclusion mode on the primary controller(s).
	 * @see OZWCommand::request()
	 */
	void startInclusion() override;

	/**
	 * @brief Cancel the inclusion mode if active.
	 * @see OZWCommand::cancelIf()
	 */
	void cancelInclusion() override;

	/**
	 * @brief Start the removal mode on the primary controller(s).
	 * @see OZWCommand::request()
	 */
	void startRemoveNode() override;

	/**
	 * @brief Cancel the removal mode if active.
	 * @see OZWCommand::cancelIf()
	 */
	void cancelRemoveNode() override;

	/**
	 * @brief Cancel the current OZW command and interrupt an active
	 * pollEvent() call.
	 */
	void interrupt() override;

	/**
	 * @brief Post the given value into the Z-Wave network. The call is non-blocking
	 * and there is no direct feedback about a successful progress. It is possible
	 * to check successful change by watching polling for events.
	 *
	 * @throws Poco::NotFoundException - when home or node are not registered
	 * @throws Poco::NotFoundException - when the command class is not
	 * registered with the node
	 * @throws Poco::InvalidArgumentException - when value cannot be set or is invalid
	 * @throws Poco::NotImplementedException - for unsupported value types
	 */
	void postValue(const ZWaveNode::Value &value);

	/**
	 * @returns label of list item for the given ValueID and a value of the list.
	 */
	std::string valueForList(const OpenZWave::ValueID &valueID, const int value);

private:
	Poco::Path m_configPath;
	Poco::Path m_userPath;
	Poco::Timespan m_pollInterval;
	bool m_intervalBetweenPolls;
	Poco::Timespan m_retryTimeout;
	bool m_assumeAwake;
	unsigned int m_driverMaxAttempts;
	std::set<uint32_t> m_controllersToReset;
	std::vector<uint8_t> m_networkKey;

	/**
	 * Homes and nodes maintained by the OZWNetwork instance.
	 */
	std::map<uint32_t, std::map<uint8_t, OZWNode>> m_homes;

	/**
	 * Set after the OZWNetwork::configure() finishes successfuly.
	 * The OZWNetwork::cleanup() does nothing, if the m_configured
	 * is false to not segfault when configure() fails.
	 */
	Poco::AtomicCounter m_configured;

	/**
	 * Lock access to the global OpenZWave::Manager instance.
	 */
	mutable Poco::FastMutex m_managerLock;

	/**
	 * Lock access to the members of OZWNetwork.
	 */
	mutable Poco::FastMutex m_lock;

	/**
	 * Initiate and maintain commands sent to the Z-Wave controller
	 * via the OpenZWave library.
	 */
	OZWCommand m_command;
	EventSource<ZWaveListener> m_eventSource;
	AsyncExecutor::Ptr m_executor;
	PeriodicRunner m_statisticsRunner;
};

}
