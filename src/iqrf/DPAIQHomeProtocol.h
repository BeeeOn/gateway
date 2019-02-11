#pragma once

#include "iqrf/DPAMappedProtocol.h"

namespace BeeeOn {

/**
 * @see https://www.iqhome.org/doc/Sensor/Protocol%20Documentation/2.0.2/Protocol%20Documentation.pdf
 */
class DPAIQHomeProtocol final : public DPAMappedProtocol {
public:
	DPAIQHomeProtocol();

	DPARequest::Ptr pingRequest(
		DPAMessage::NetworkAddress node) const override;

	DPARequest::Ptr dpaModulesRequest(
		DPAMessage::NetworkAddress node) const override;
	std::list<ModuleType> extractModules(
		const std::vector<uint8_t> &message) const override;

	DPARequest::Ptr dpaValueRequest(
		DPAMessage::NetworkAddress node,
		const std::list<ModuleType> &type) const override;
	SensorData parseValue(
		const std::list<ModuleType> &modules,
		const std::vector<uint8_t> &msg) const override;

	DPARequest::Ptr dpaProductInfoRequest(
		DPAMessage::NetworkAddress address) const override;

	/**
	 * @brief Extracts product and vendor name from given message.
	 * Given message contains string with two parts (product code and
	 * hardware revision).
	 */
	ProductInfo extractProductInfo(
		const std::vector<uint8_t> &msg,
		uint16_t hwPID) const override;
};

}
