#pragma once

#include <list>

#include <Poco/Mutex.h>
#include <Poco/SharedPtr.h>
#include <Poco/Timespan.h>
#include <Poco/URI.h>
#include <Poco/DOM/Node.h>
#include <Poco/DOM/NodeIterator.h>

#include "model/DeviceID.h"
#include "model/ModuleID.h"
#include "model/ModuleType.h"
#include "model/SensorData.h"
#include "util/Loggable.h"

namespace BeeeOn {

/**
 * @brief Abstract class representing generic BelkinWemo device.
 */
class BelkinWemoDevice : protected Loggable {
public:
	typedef Poco::SharedPtr<BelkinWemoDevice> Ptr;

	BelkinWemoDevice(const DeviceID& id);
	virtual ~BelkinWemoDevice();

	DeviceID deviceID() const;

	virtual bool requestModifyState(const ModuleID& moduleID, const double value) = 0;
	virtual SensorData requestState() = 0;

	virtual std::list<ModuleType> moduleTypes() const = 0;
	virtual std::string name() const = 0;
	virtual Poco::FastMutex& lock();

	/**
	 * @brief Finds the first node with the given name
	 * and returns it's value. When the value of node si empty
	 * returns NULL. If the node with the given name does not exist
	 * Poco::NotFoundException is raised.
	 */
	static Poco::XML::Node* findNode(Poco::XML::NodeIterator& iterator, const std::string& name);

	/**
	 * @brief Finds all nodes with the given name
	 * and returns their values.
	 */
	static std::list<Poco::XML::Node*> findNodes(Poco::XML::NodeIterator& iterator, const std::string& name);

protected:
	const DeviceID m_deviceId;
	Poco::FastMutex m_lock;
};

}
