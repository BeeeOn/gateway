#pragma once

#include <cstdint>

#include <Driver.h>

namespace BeeeOn {

/**
 * Statistics from ZWave network from USB driver.
 *
 * @see https://github.com/OpenZWave/open-zwave/blob/master/cpp/src/Driver.h (struct DriverData)
 * @see http://www.openzwave.com/dev/classOpenZWave_1_1Manager.html (GetDriverStatistics())
 */
class ZWaveDriverEvent {
public:
	/**
	 * Creates statistics data from node.
	 */
	ZWaveDriverEvent(const OpenZWave::Driver::DriverData &data);

	uint32_t SOAFCount() const;
	uint32_t ACKWaiting() const;
	uint32_t readAborts() const;
	uint32_t badChecksum() const;
	uint32_t readCount() const;
	uint32_t writeCount() const;
	uint32_t CANCount() const;
	uint32_t NAKCount() const;
	uint32_t ACKCount() const;
	uint32_t OOFCount() const;
	uint32_t dropped() const;
	uint32_t retries() const;
	uint32_t callbacks() const;
	uint32_t badroutes() const;
	uint32_t noACK() const;
	uint32_t netBusy() const;
	uint32_t notIdle() const;
	uint32_t nonDelivery() const;
	uint32_t routedBusy() const;
	uint32_t broadcastReadCount() const;
	uint32_t broadcastWriteCount() const;

private:
	uint32_t m_SOFCnt;            // Number of SOF bytes received
	uint32_t m_ACKWaiting;        // Number of unsolicited messages while waiting for an ACK
	uint32_t m_readAborts;        // Number of times read were aborted due to timeouts
	uint32_t m_badChecksum;       // Number of bad checksums
	uint32_t m_readCnt;           // Number of messages successfully read
	uint32_t m_writeCnt;          // Number of messages successfully sent
	uint32_t m_CANCnt;            // Number of CAN bytes received
	uint32_t m_NAKCnt;            // Number of NAK bytes received
	uint32_t m_ACKCnt;            // Number of ACK bytes received
	uint32_t m_OOFCnt;            // Number of bytes out of framing
	uint32_t m_dropped;           // Number of messages dropped & not delivered
	uint32_t m_retries;           // Number of retransmitted messages
	uint32_t m_callbacks;         // Number of unexpected callbacks
	uint32_t m_badroutes;         // Number of failed messages due to bad route response
	uint32_t m_noACK;             // Number of no ACK returned errors
	uint32_t m_netbusy;           // Number of network busy/failure messages
	uint32_t m_notidle;           // Number of not idle messages
	uint32_t m_nondelivery;       // Number of messages not delivered to network
	uint32_t m_routedbusy;        // Number of messages received with routed busy status
	uint32_t m_broadcastReadCnt;  // Number of broadcasts read
	uint32_t m_broadcastWriteCnt; // Number of broadcasts sent
};

}
