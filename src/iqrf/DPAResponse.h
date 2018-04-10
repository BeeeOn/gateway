#pragma once

#include <string>

#include "iqrf/DPAMessage.h"

namespace BeeeOn {

/**
 * @brief Each response contains a header with:
 *
 *  - NADR(2B) - network address
 *  - PNUM(1B)- peripherals number
 *  - CMD(1B) - command identification
 *  - HWPID(2B) - hw profile
 *  - ErrN(1B) - DPA error code
 *  - DpaValue(1B) - local node's value
 *  - PData(max 59B) - peripheral data, data that was received
 */
class DPAResponse : public DPAMessage {
public:
	typedef Poco::SharedPtr<DPAResponse> Ptr;

	/**
	 * @see https://github.com/iqrfsdk/iqrf-daemon/wiki/JsonStructureDpa-v1#response--status-codes
	 * @see https://github.com/iqrfsdk/clibdpa/blob/master/Dpa/DPA.h
	 */
	void setErrorCode(uint8_t errCode);
	uint8_t errorCode() const;

	void setDPAValue(uint8_t dpaValue);
	uint8_t dpaValue() const;

	/**
	 * @brief Converts the header items and peripheral data
	 * to string that is divided by dots.
	 */
	virtual std::string toDPAString() const override;

	/**
	 * @brief Parses given message into general or specific response.
	 */
	static DPAResponse::Ptr fromRaw(const std::string &data);

protected:
	DPAResponse();

	DPAResponse(
		DPAMessage::NetworkAddress node,
		uint8_t pNumber,
		uint8_t pCommand,
		uint16_t hwPID,
		const std::vector<uint8_t> &pData,
		uint8_t errorCode,
		uint8_t dpaValue
	);

private:
	uint8_t m_errorCode;
	uint8_t m_dpaValue;
};

}
