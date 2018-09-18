#pragma once

#include <cstdint>
#include <map>

namespace BeeeOn {

/**
 * Statistics from ZWave network from one device.
 *
 * @see https://github.com/OpenZWave/open-zwave/blob/master/cpp/src/Node.h (struct NodeData)
 * @see http://www.openzwave.com/dev/classOpenZWave_1_1Manager.html (GetNodeStatistics())
 */
class ZWaveNodeEvent {
public:
	/**
	 * Creates statistics data from node.
	 */
	ZWaveNodeEvent(
		const std::map<std::string, uint32_t> &stats,
		uint8_t nodeID);

	uint32_t sentCount() const;
	uint32_t sentFailed() const;
	uint32_t retries() const;
	uint32_t receivedCount() const;
	uint32_t receiveDuplications() const;
	uint32_t receiveUnsolicited() const;
	uint32_t lastRequestRTT() const;
	uint32_t lastResponseRTT() const;
	uint32_t averageRequestRTT() const;
	uint32_t averageResponseRTT() const;
	uint32_t quality() const;

	/**
	 * ZWave device (node) identification.
	 */
	uint8_t nodeID() const;

protected:
	uint32_t lookup(const std::string &key) const;

private:
	uint8_t m_nodeID;                // Identification of Z-Wave device.
	std::map<std::string, uint32_t> m_stats;
};

}
