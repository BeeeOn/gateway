#pragma once

#include <string>

#include "model/DeviceID.h"

namespace BeeeOn {

class ZWaveUtil {
public:
	ZWaveUtil() = delete;
	ZWaveUtil(const ZWaveUtil &) = delete;
	ZWaveUtil(const ZWaveUtil &&) = delete;
	~ZWaveUtil() = delete;

	static std::string commandClass(const uint8_t cclass);
	static std::string commandClass(const uint8_t cclass, const uint8_t index);

	/**
	 * It builds DeviceID from homeID, nodeID.
	 *
	 * DeviceID contains:
	 *  - 8b prefix
	 *  - 16b empty
	 *  - 32b homeID
	 *  - 8b nodeID
	 */
	static DeviceID buildID(uint32_t homeID, uint8_t nodeID);
};

}
