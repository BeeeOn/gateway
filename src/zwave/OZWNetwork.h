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

class OZWNetwork :
	public HotplugListener,
	public AbstractZWaveNetwork {
	friend class OZWCommand;
public:
	OZWNetwork();
	~OZWNetwork();

	void setConfigPath(const std::string &path);
	void setUserPath(const std::string &path);
	void setPollInterval(const Poco::Timespan &interval);
	void setIntervalBetweenPolls(bool enable);
	void setRetryTimeout(const Poco::Timespan &timeout);
	void setAssumeAwake(bool awake);
	void setDriverMaxAttempts(int attempts);
	void setNetworkKey(const std::list<std::string> &bytes);
	void setStatisticsInterval(const Poco::Timespan &interval);
	void setControllersToReset(const std::list<std::string> &homes);

	void setExecutor(AsyncExecutor::Ptr executor);
	void registerListener(ZWaveListener::Ptr listener);

	void onNotification(const OpenZWave::Notification *n);

	void onAdd(const HotplugEvent &event) override;
	void onRemove(const HotplugEvent &event) override;

	void configure();
	void cleanup();

protected:
	void checkDirectory(const Poco::Path &path);
	void prepareDirectory(const Poco::Path &path);
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
	std::map<uint32_t, std::map<uint8_t, ZWaveNode>> m_homes;
	Poco::AtomicCounter m_configured;
	mutable Poco::FastMutex m_managerLock;
	mutable Poco::FastMutex m_lock;
	OZWCommand m_command;
	EventSource<ZWaveListener> m_eventSource;
	AsyncExecutor::Ptr m_executor;
	PeriodicRunner m_statisticsRunner;
};

}