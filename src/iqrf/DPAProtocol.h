#pragma once

#include <list>
#include <string>
#include <vector>

#include <Poco/SharedPtr.h>

#include "iqrf/DPAMessage.h"
#include "iqrf/DPARequest.h"
#include "model/ModuleType.h"
#include "model/SensorData.h"

namespace BeeeOn {

/**
 * @brief The class provides interface for obtaining of measured
 * data, for detecting of supported modules that can be specific
 * for general or some own protocol.
 */
class DPAProtocol {
public:
	typedef Poco::SharedPtr<DPAProtocol> Ptr;

	/**
	 * @brief Vendor and product name for each paired device. It can be
	 * filled from IQRF repository or statically from code.
	 */
	struct ProductInfo {
		const std::string vendorName;
		const std::string productName;
	};

	virtual ~DPAProtocol();

	/**
	 * @brief DPA request for detecting that specific device communicates
	 * using the same protocols as implemented protocol.
	 *
	 * @returns DPARequest that can be used as a ping message.
	 */
	virtual DPARequest::Ptr pingRequest(
		DPAMessage::NetworkAddress address) const = 0;

	/**
	 * @returns DPA request for detecting of product info about specific
	 * device (vendor name and product name).
	 */
	virtual DPARequest::Ptr dpaProductInfoRequest(
		DPAMessage::NetworkAddress address) const = 0;

	/**
	 * @brief Obtaining of information from received response on the
	 * dpaProductInfoRequest().
	 *
	 * @returns product information as encoded in the given message
	 */
	virtual ProductInfo extractProductInfo(
		const std::vector<uint8_t> &msg,
		uint16_t hwPID) const = 0;

	/**
	 * @returns DPA request to detect available modules on the
	 * specific device.
	 */
	virtual DPARequest::Ptr dpaModulesRequest(
		DPAMessage::NetworkAddress node) const = 0;

	/**
	 * @returns list of module types encoded in the given message
	 */
	virtual std::list<ModuleType> extractModules(
		const std::vector<uint8_t> &message) const = 0;

	/**
	 * @returns DPA request to obtain measured values from a specific
	 * IQRF node.
	 */
	virtual DPARequest::Ptr dpaValueRequest(
		DPAMessage::NetworkAddress node,
		const std::list<ModuleType> &type) const = 0;

	/**
	 * @brief Obtains measured values from the given byte message.
	 * The contents of the message must conform with the list of
	 * module types.
	 *
	 * @returns measured values encoded in the given message
	 */
	virtual SensorData parseValue(
		const std::list<ModuleType> &modules,
		const std::vector<uint8_t> &msg) const = 0;
};

}
