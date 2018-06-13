#pragma once

#include "iqrf/DPAResponse.h"

namespace BeeeOn {

class DPAOSPeripheralInfoResponse final : public DPAResponse {
public:
	typedef Poco::SharedPtr<DPAOSPeripheralInfoResponse> Ptr;

	/**
	 * @returns Module ID - 4B identification code unique for each TR module.
	 */
	uint32_t mid() const;

	/**
	 * @returns RSSI value ([dBm]) of incoming RF signal (11dBm - 11dBm).
	 *
	 * @see https://www.iqrf.org/IQRF-OS-Reference-guide/
	 * @see http://iqrf.org/weben/downloads.php?id=333
	 */
	int8_t rssi() const;

	/**
	 * @returns Power supply measurement (up to 3.84V),
	 * low battery state: voltage < 2.93V.
	 *
	 * @see https://www.iqrf.org/IQRF-OS-Reference-guide/
	 */
	double supplyVoltage() const;

	double percentageSupplyVoltage() const;
};

}
