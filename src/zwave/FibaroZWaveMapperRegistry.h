#pragma once

#include "zwave/SpecificZWaveMapperRegistry.h"

namespace BeeeOn {

class FibaroZWaveMapperRegistry : public SpecificZWaveMapperRegistry {
public:
	FibaroZWaveMapperRegistry();

private:
	class FGK101Mapper : public Mapper {
	public:
		FGK101Mapper(
			const ZWaveNode::Identity &id,
			const std::string &product);

		std::list<ModuleType> types() const override;
		SensorValue convert(const ZWaveNode::Value &value) const override;
	};

	class FGSD002Mapper : public Mapper {
	public:
		FGSD002Mapper(
			const ZWaveNode::Identity &id,
			const std::string &product);

		std::list<ModuleType> types() const override;
		SensorValue convert(const ZWaveNode::Value &value) const override;
	};
};

}
