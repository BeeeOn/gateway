#pragma once

#include "zwave/SpecificZWaveMapperRegistry.h"

namespace BeeeOn {

/**
 * @brief Support PIR sensor from different manufacturers that seems to
 * be based on the same PCB marked as ST02L1(V1), 20140514 RoHS.
 *
 * It covers sensors of 3 categories:
 *
 * - 3-in-1 PIR, Temperature, Illumination
 * - 3-in-1 Door/Window, Temperature, Illumination
 * - 4-in-1 PIR, Door/Window, Temperature, Illumination
 *
 * Each category has a corresponding Mapper implementation that can be
 * used in the specMap property as:
 *
 * - 3-in-1-pir
 * - 3-in-1
 * - 4-in-1
 */
class ST02L1ZWaveMapperRegistry : public SpecificZWaveMapperRegistry {
public:
	ST02L1ZWaveMapperRegistry();

private:
	/**
	 * @brief Convert values that generically apply for all 3 categories
	 * of these sensors: temperature, illumination, battery, tempering.
	 */
	static SensorValue convert(const ZWaveNode::Value &value);

	/**
	 * @brief Implements 3-in-1 variant of the sensor. Based on the argument
	 * pirVariant given during instantiation, it implements either
	 * PIR-variant or Door/Window-variant.
	 */
	class Device3in1Mapper : public Mapper {
	public:
		Device3in1Mapper(
			const ZWaveNode::Identity &id,
			const std::string &product,
			bool pirVariant = false);

		std::list<ModuleType> types() const override;
		SensorValue convert(const ZWaveNode::Value &value) const override;

	private:
		bool m_pirVariant;
	};

	/**
	 * @brief Helper class that calls its parent as Device3in1Mapper(true).
	 */
	class Device3in1WithPIRMapper : public Device3in1Mapper {
	public:
		Device3in1WithPIRMapper(
			const ZWaveNode::Identity &id,
			const std::string &product);
	};

	/**
	 * @brief Implements 4-in-1 variant of the sensor.
	 */
	class Device4in1Mapper : public Mapper {
	public:
		Device4in1Mapper(
			const ZWaveNode::Identity &id,
			const std::string &product);

		std::list<ModuleType> types() const override;
		SensorValue convert(const ZWaveNode::Value &value) const override;
	};
};

}
