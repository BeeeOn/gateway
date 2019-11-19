#pragma once

#include <vector>

#include <Poco/SharedPtr.h>

#include "iqrf/DPAResponse.h"
#include "iqrf/DPARequest.h"

namespace BeeeOn {

/**
 * Stores information about IQRF DPA packets.
 */
class IQRFEvent {
public:
	typedef Poco::SharedPtr<IQRFEvent> Ptr;

	IQRFEvent(DPARequest::Ptr const request);
	IQRFEvent(DPAResponse::Ptr const response);


	uint16_t networkAddress() const;
	uint8_t peripheralNumber() const;
	uint8_t commandCode() const;
	uint16_t HWProfile() const;
	std::vector<uint8_t> payload() const;

	uint8_t direction() const;
	uint8_t size() const;

private:
	uint16_t m_NetworkAddress;
	uint8_t m_pNumber;
	uint8_t m_pCommand;
	uint16_t m_hwPID;
	std::vector<uint8_t> m_peripheralData;

	uint8_t m_direction;
};

}
