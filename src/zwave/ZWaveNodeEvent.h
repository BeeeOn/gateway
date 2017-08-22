#pragma once

#include <cstdint>

#include <Node.h>

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
		const OpenZWave::Node::NodeData &data, uint8_t nodeID);

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

private:
	uint8_t m_nodeID;                // Identification of Z-Wave device.
	uint32_t m_sentCnt;              // Number of messages sent from this node.
	uint32_t m_sentFailed;           // Number of sent messages failed
	uint32_t m_retries;              // Number of message retries
	uint32_t m_receivedCnt;          // Number of messages received from this node.
	uint32_t m_receivedDups;         // Number of duplicated messages received;
	uint32_t m_receivedUnsolicited;  // Number of messages received unsolicited
	uint32_t m_lastRequestRTT;       // Last message request RTT
	uint32_t m_lastResponseRTT;      // Last message response RTT
	uint32_t m_averageRequestRTT;    // Average Request round trip time (ms).
	uint32_t m_averageResponseRTT;   // Average Response round trip time.
	uint32_t m_quality;              // Node quality measure
};

}
