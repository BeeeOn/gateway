#pragma once

#include <cstdint>
#include <map>

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
	ZWaveDriverEvent(const std::map<std::string, uint32_t> &stats);

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

protected:
	uint32_t lookup(const std::string &key) const;

private:
	std::map<std::string, uint32_t> m_stats;
};

}
