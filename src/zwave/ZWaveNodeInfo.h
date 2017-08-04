#pragma once

#include <list>
#include <set>
#include <string>

#include <openzwave/Notification.h>

#include "model/DeviceID.h"
#include "model/ModuleID.h"
#include "model/ModuleType.h"

namespace BeeeOn {

/**
 * This class contains information about Z-Wave device (node),
 * for example: name of product, vendor and their identification.
 * It also contains identification of device (node) in BeeeOn system
 * (DeviceID) and values that can be measured (OpenZWave::ValueID).
 */
class ZWaveNodeInfo {
public:
	typedef std::pair<OpenZWave::ValueID, ModuleType> ZWaveValuePair;

	ZWaveNodeInfo();

	static ZWaveNodeInfo build(uint32_t homeID, uint8_t nodeID);

	void setPaired(bool paired);
	bool paired() const;

	void setVendorName(const std::string &name);
	std::string vendorName() const;

	void setVendorID(uint32_t id);
	uint32_t vendorID() const;

	void setProductName(const std::string &name);
	std::string productName() const;

	void setProductID(uint32_t id);
	uint32_t productID() const;

	void setPolled(bool polled);
	bool polled() const;

	void setDeviceID(const DeviceID &deviceID);
	DeviceID deviceID() const;

	void addValueID(const OpenZWave::ValueID &valueID, const ModuleType &type);
	std::vector<ZWaveValuePair> valueIDs() const;

	ModuleID findModuleID(const OpenZWave::ValueID &valueID) const;
	OpenZWave::ValueID findValueID(const unsigned int &index) const;
	ModuleType findModuleType(const OpenZWave::ValueID &valueID) const;

	std::list<ModuleType> moduleTypes() const;

private:
	std::vector<ZWaveValuePair> m_zwaveValues;
	bool m_polled;
	bool m_paired;
	std::string m_vendorName;
	uint32_t m_vendorID;
	std::string m_productName;
	uint32_t m_productID;
	DeviceID m_deviceID;
};

}
