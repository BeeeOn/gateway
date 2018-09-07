#pragma once

#include <map>
#include <set>
#include <string>

#include <Poco/Clock.h>
#include <Poco/Mutex.h>
#include <Poco/Timespan.h>

#include "core/DeviceManager.h"
#include "model/SensorValue.h"
#include "util/DelayedAsyncWork.h"
#include "zwave/ZWaveMapperRegistry.h"
#include "zwave/ZWaveNetwork.h"
#include "zwave/ZWaveNode.h"

namespace BeeeOn {

/**
 * @brief ZWaveDeviceManager implements the logical layer on top of
 * the ZWaveNetwork interface. It adapts the Z-Wave specifics to the
 * BeeeOn system with the help of ZWaveMapperRegistry::Mapper.
 *
 * The Z-Wave provides a dynamic discovery progress where it is possible
 * to ask the discovered devices for their specifics modules. This process
 * can however be time consuming. It can have multiple steps:
 *
 * 1. Discover new Z-Wave node.
 * 2. Get few details like vendor ID, product ID.
 * 3. Probe features of the Z-Wave node.
 *
 * The steps 1 and 2 are usually fast enough, however, the step 3 can take
 * more than a minute. Each Z-Wave node, to be operational, must recognized
 * by a ZWaveMapperRegistry and a specific Mapper instance must be assigned
 * to it. Certain Mapper implementions can work just after the step 2,
 * other might need the step 3 to complete.
 *
 * A Z-Wave node is considered as working when a Mapper is resolved for it.
 * If no Mapper is resolved such Z-Wave node is dropped until an update of
 * its details comes from the underlying ZWaveNetwork.
 */
class ZWaveDeviceManager : public DeviceManager {
public:
	/**
	 * @brief High-level representation of a Z-Wave device. It enriches
	 * the ZWaveNode for information needed by the manager and the BeeeOn
	 * system. This means especially the Mapper instance.
	 */
	class Device {
	public:
		Device(const ZWaveNode &node);

		DeviceID id() const;

		std::string product() const;
		std::string vendor() const;

		/**
		 * @brief Update the underlying ZWaveNode instance by the
		 * given one. Their identities must match.
		 *
		 * If the underlying node is already queried, any attempt to
		 * update it by a ZWaveNode instance which is not queried would
		 * be ignored.
		 */
		void updateNode(const ZWaveNode &node);
		const ZWaveNode &node() const;

		/**
		 * @brief Try to resolve Mapper for this Device based on
		 * the ZWaveNode contents unless a mapper is already resolved.
		 * @returns true if a mapper is available
		 */
		bool resolveMapper(ZWaveMapperRegistry::Ptr registry);

		ZWaveMapperRegistry::Mapper::Ptr mapper() const;

		void setRefresh(const Poco::Timespan &refresh);
		Poco::Timespan refresh() const;

		/**
		 * @returns list of types the devices provides to the BeeeOn system
		 */
		std::list<ModuleType> types() const;

		/**
		 * @returns convert the given Z-Wave value to BeeeOn sensor value
		 */
		SensorValue convert(const ZWaveNode::Value &value) const;

		std::string toString() const;

	private:
		ZWaveNode m_node;
		ZWaveMapperRegistry::Mapper::Ptr m_mapper;
		Poco::Timespan m_refresh;
	};

	typedef std::map<DeviceID, Device> DeviceMap;
	typedef std::map<ZWaveNode::Identity, DeviceMap::iterator> ZWaveNodeMap;

	ZWaveDeviceManager();

	/**
	 * @brief Configure ZWaveNetwork to be used by this manager.
	 */
	void setNetwork(ZWaveNetwork::Ptr network);

	/**
	 * @brief Set registry that are able to resolve the Z-Wave devices
	 * specific features.
	 */
	void setRegistry(ZWaveMapperRegistry::Ptr registry);

	/**
	 * @brief Set maximal time window in which new devices are
	 * reported since the start of the inclusion mode.
	 *
	 * The dispatch time usually needs to be longer than the listen
	 * duration as received from the server because the node discovery
	 * is sometimes very slow.
	 */
	void setDispatchDuration(const Poco::Timespan &duration);

	/**
	 * @brief Set poll timeout of the main loop polling the configured
	 * ZWaveNetwork instance.
	 */
	void setPollTimeout(const Poco::Timespan &timeout);

	/**
	 * @brief Run the loop that receives events from the configured
	 * ZWaveNetwork instance. The loop receives information about
	 * sensor data, new nodes, nodes removals, discovery, etc.
	 */
	void run() override;

	/**
	 * @brief Stop the polling loop.
	 */
	void stop() override;

protected:
	void handleAccept(const DeviceAcceptCommand::Ptr cmd) override;
	AsyncWork<>::Ptr startDiscovery(const Poco::Timespan &duration) override;
	AsyncWork<std::set<DeviceID>>::Ptr startUnpair(
		const DeviceID &id,
		const Poco::Timespan &timeout) override;
	AsyncWork<double>::Ptr startSetValue(
			const DeviceID &id,
			const ModuleID &module,
			const double value,
			const Poco::Timespan &timeout) override;

	/**
	 * @brief Dispatch all registered devices that are not paired to the
	 * remote server.
	 */
	void dispatchUnpaired();

	/**
	 * @brief Dispatch the given device to the remote server.
	 *
	 * If the device is dispatchable (it is not paired nor it is a controller)
	 * it is not shipped.
	 *
	 * If the given parameter _enabled_ is false, no dispatching would
	 * occur as we assume that discovery mode is disabled at that moment.
	 * The purpose of the _enabled_ is that we are likely to first check
	 * whether the device is dispatchable and than to check whether discovery
	 * is allowed.
	 */
	void dispatchDevice(const Device &device, bool enabled = true);

	/**
	 * @brief Access cached device IDs of the Z-Wave nodes removed by
	 * the recent unpair process. The call clears that cache.
	 *
	 * The method startUnpair() initiates the Z-Wave node removal process.
	 * While in progress, the main loop would recieve information about
	 * Z-Wave nodes being removed from the Z-Wave network. The devices are
	 * stored in a temporary cache that can be accessed by this method.
	 *
	 * The temporary cache is to be used only during the unpair process.
	 */
	std::set<DeviceID> recentlyUnpaired();

	/**
	 * @brief Process a Z-Wave value received from the Z-Wave network.
	 * If there is a paired device for that value and there is a conversion
	 * available for that value, the value is shipped via Distributor.
	 *
	 * The method processes also values that are not to be shipped, like
	 * the refresh time.
	 */
	void processValue(const ZWaveNode::Value &value);

	/**
	 * @brief If the given node is fully resolvable (we can determine
	 * its Mapper instance), it is registered as a new Device and dispatched
	 * as a new device. Otherwise, such node is ignored.
	 */
	void newNode(const ZWaveNode &node, bool dispatch);
	void newNodeUnlocked(const ZWaveNode &node, bool dispatch);

	/**
	 * @brief If the given node is already registered, update information about
	 * it and if it is not paired, dispatch it as a new device (or updated device).
	 * If the node is not registered, the newNode() is called instead.
	 *
	 * @see newNode()
	 */
	void updateNode(const ZWaveNode &node, bool dispatch);

	/**
	 * @brief The given node is considered to be unpaired, its cached data
	 * is deleted.
	 */
	void removeNode(const ZWaveNode &node);

	/**
	 * @brief Register a device to be available for the BeeeOn system. The device
	 * must have a resolved mapper. Pairing status is irrelevant for this method
	 * and not affected.
	 */
	void registerDevice(const Device &device);

	/**
	 * @brief Unregister the device pointed to by the iterator from being available
	 * to the BeeeOn system. Pairing status is irrelevant for this method and not
	 * affected.
	 *
	 * @returns shallow copy of the removed device
	 */
	Device unregisterDevice(ZWaveNodeMap::iterator it);

	/**
	 * @brief Helper method to stop the Z-Wave inclusion mode.
	 */
	void stopInclusion();

	/**
	 * @brief Helper method to stop the Z-Wave node removal mode.
	 */
	void stopRemoveNode();

private:
	ZWaveNetwork::Ptr m_network;
	ZWaveMapperRegistry::Ptr m_registry;
	Poco::Timespan m_dispatchDuration;
	Poco::Timespan m_pollTimeout;

	/**
	 * Cache of devices discovered by Z-Wave with a resolved Mapper instance.
	 */
	DeviceMap m_devices;

	/**
	 * Cache of relation between ZWaveNode::Identity and the associated Device.
	 */
	ZWaveNodeMap m_zwaveNodes;

	/**
	 * Temporary cache of recently removed Z-Wave nodes (their device IDs).
	 * The startUnpair() method clears the cache and it is filled again by
	 * the main loop. When the unpair process finishes, the cache should be
	 * treated as invalid.
	 */
	std::set<DeviceID> m_recentlyUnpaired;

	/**
	 * Protect access to m_devices, m_zwaveNodes, m_recentlyUnpaired.
	 */
	Poco::FastMutex m_lock;

	/**
	 * Current AsyncWork for the Z-Wave inclusion mode. Only 1 inclusion mode
	 * can be active at a time. A new instance is created on each startDiscovery().
	 */
	DelayedAsyncWork<>::Ptr m_inclusionWork;

	/**
	 * Current AsyncWork for the Z-Wave node removal mode. Only 1 removal mode
	 * can be active at a time. A new instance is created on each startUnpair().
	 */
	DelayedAsyncWork<std::set<DeviceID>>::Ptr m_removeNodeWork;
};

}
