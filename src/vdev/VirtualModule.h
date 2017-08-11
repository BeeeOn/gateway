#ifndef BEEEON_VIRTUAL_MODULE_H
#define BEEEON_VIRTUAL_MODULE_H

#include <list>

#include <Poco/Nullable.h>
#include <Poco/SharedPtr.h>

#include "core/Result.h"
#include "model/ModuleID.h"
#include "model/ModuleType.h"
#include "util/Loggable.h"
#include "util/ValueGenerator.h"

namespace BeeeOn {

class VirtualModule {
public:
	typedef Poco::SharedPtr<VirtualModule> Ptr;

	enum Reaction {
		REACTION_SUCCESS,
		REACTION_FAILURE,
		REACTION_NONE,
	};

	VirtualModule(const ModuleType &moduleType);
	~VirtualModule();

	void setModuleID(const ModuleID &moduleID);
	ModuleID moduleID() const;

	void setModuleType(const ModuleType &moduleType);
	ModuleType moduleType() const;

	void setMin(double min);
	double min() const;

	void setMax(double max);
	double max() const;

	void modifyValue(double value, Result::Ptr result);

	void setGenerator(const std::string &generator);
	Poco::SharedPtr<ValueGenerator> generator() const;
	bool generatorEnabled() const;

	SensorValue generate();

	void setReaction(const std::string &reaction);
	Reaction reaction() const;

private:
	ModuleID m_moduleID;
	ModuleType m_moduleType;
	Poco::Nullable<double> m_min;
	Poco::Nullable<double> m_max;
	std::list<Poco::SharedPtr<ValueGenerator>> m_generators;
	Reaction m_reaction;
	double m_value;
};

}

#endif
