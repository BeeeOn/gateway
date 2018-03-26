#ifndef BEEEON_PHILIPS_HUE_BRIDGE_INFO_H
#define BEEEON_PHILIPS_HUE_BRIDGE_INFO_H

#include <string>

#include "net/MACAddress.h"

namespace BeeeOn {

/**
 * Provides information about Philips Hue bridge.
 */
class PhilipsHueBridgeInfo {
public:
	PhilipsHueBridgeInfo();

	std::string name() const;
	std::string dataStoreVersion() const;
	std::string swVersion() const;
	std::string apiVersion() const;
	MACAddress mac() const;
	std::string bridgeId() const;
	bool factoryNew() const;
	std::string modelId() const;

	static PhilipsHueBridgeInfo buildBridgeInfo(
		const std::string& response);

private:
	std::string m_name;
	std::string m_dataStoreVersion;
	std::string m_swVersion;
	std::string m_apiVersion;
	MACAddress m_mac;
	std::string m_bridgeId;
	bool m_factoryNew;
	std::string m_modelId;
};

}

#endif
