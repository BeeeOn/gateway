#pragma once

#include <utility>

#include "util/Loggable.h"

#include "iqrf/DPAProtocol.h"
#include "iqrf/IQRFTypeMappingParser.h"

namespace BeeeOn {

/**
 * @brief Map the IQRFType-specific data to the BeeeOn-specific ones.
 */
class DPAMappedProtocol : public DPAProtocol, protected Loggable {
public:
	DPAMappedProtocol(
		const std::string &mappingGroup,
		const std::string &techNode
	);

	/**
	 * @brief Load XML file with the types mapping between IQRF and BeeeOn.
	 */
	void loadTypesMapping(const std::string &file);
	void loadTypesMapping(std::istream &in);

	std::list<ModuleType> extractModules(
		const std::vector<uint8_t> &message) const override;

	SensorData parseValue(
		const std::list<ModuleType> &modules,
		const std::vector<uint8_t> &msg) const override;

protected:
	/**
	 * @brief Reads info about value based on IQRFType and converts
	 * measured value to SensorValue.
	 */
	SensorValue extractSensorValue(
		const ModuleID &moduleID,
		const IQRFType &type,
		const uint16_t value) const;

	/**
	 * @brief Find module type by IQRF type id.
	 */
	ModuleType findModuleType(uint8_t id) const;

	/**
	 * @brief Find iqrf type by IQRF type id.
	 */
	IQRFType findIQRFType(uint8_t id) const;

private:
	std::string m_mappingGroup;
	std::string m_techNode;
	std::map<uint8_t, ModuleType> m_moduleTypes;
	std::map<uint8_t, IQRFType> m_iqrfTypes;
};

}
