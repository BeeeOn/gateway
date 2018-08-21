#pragma once

#include "zwave/SpecificZWaveMapperRegistry.h"

namespace BeeeOn {

class ClimaxZWaveMapperRegistry : public SpecificZWaveMapperRegistry {
public:
	ClimaxZWaveMapperRegistry();

private:
	class DC23ZWMapper : public Mapper {
	public:
		DC23ZWMapper(
			const ZWaveNode::Identity &id,
			const std::string &product);

		std::list<ModuleType> types() const override;
		SensorValue convert(const ZWaveNode::Value &value) const override;
	};
};

}

