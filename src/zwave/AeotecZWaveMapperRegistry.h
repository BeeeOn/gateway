#pragma once

#include "zwave/SpecificZWaveMapperRegistry.h"

namespace BeeeOn {

class AeotecZWaveMapperRegistry : public SpecificZWaveMapperRegistry {
public:
	AeotecZWaveMapperRegistry();

private:
	class ZW100Mapper : public Mapper {
	public:
		ZW100Mapper(
			const ZWaveNode::Identity &id,
			const std::string &product);

		std::list<ModuleType> types() const override;
		SensorValue convert(const ZWaveNode::Value &value) const override;
	};
};

}
