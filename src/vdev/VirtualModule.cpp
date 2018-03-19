#include <string>

#include <Poco/NumberParser.h>
#include <Poco/Timespan.h>

#include "model/SensorData.h"
#include "vdev/VirtualModule.h"

using namespace BeeeOn;
using namespace Poco;
using namespace std;

VirtualModule::VirtualModule(const ModuleType &moduleType):
	m_moduleType(moduleType)
{
}

VirtualModule::~VirtualModule()
{
}

SensorValue VirtualModule::generate()
{
	return SensorValue(moduleID(), m_generators.back()->next());
}

void VirtualModule::setMin(double min)
{
	m_min = min;
}

double VirtualModule::min() const
{
	return m_min.value();
}

void VirtualModule::setMax(double max)
{
	m_max = max;
}

double VirtualModule::max() const
{
	return m_max.value();
}

void VirtualModule::setGenerator(const string &generator)
{
	if (generator.empty()) {
		return;
	}
	else if (generator == "random") {
		m_generators.clear();
		m_generators.emplace_back(new RandomGenerator);
		m_generators.emplace_back(new RangeGenerator(*m_generators.front(), min(), max()));
	}
	else if (generator == "sin") {
		m_generators.clear();
		m_generators.emplace_back(new SinGenerator);
		m_generators.emplace_back(new RangeGenerator(*m_generators.front(), min(), max()));
	}
	else {
		double tmp;
		if (NumberParser::tryParseFloat(generator, tmp)) {
			m_generators.clear();
			m_generators.emplace_back(new ConstGenerator(tmp));
		}
		else {
			throw InvalidArgumentException("invalid generator value: " + generator);
		}
	}
}

bool VirtualModule::generatorEnabled() const
{
	return !m_generators.empty();
}

void VirtualModule::setReaction(const string &reaction)
{
	if (reaction == "success")
		m_reaction = VirtualModule::REACTION_SUCCESS;
	else if (reaction == "failure")
		m_reaction = VirtualModule::REACTION_FAILURE;
	else if (reaction == "none")
		m_reaction = VirtualModule::REACTION_NONE;
	else
		throw InvalidArgumentException("invalid reaction: " + reaction);
}

VirtualModule::Reaction VirtualModule::reaction() const
{
	return m_reaction;
}

void VirtualModule::setModuleType(const ModuleType &moduleType)
{
	m_moduleType = moduleType;
}

ModuleType VirtualModule::moduleType() const
{
	return m_moduleType;
}

void VirtualModule::setModuleID(const ModuleID &moduleID)
{
	m_moduleID = moduleID;
}

ModuleID VirtualModule::moduleID() const
{
	return m_moduleID;
}

bool VirtualModule::modifyValue(double value)
{
	if (reaction() == REACTION_SUCCESS) {
		m_value = value;
		return true;
	}
	else if (reaction() == REACTION_FAILURE) {
		return false;
	}
	else {
		throw IllegalStateException(
			"module "
			+ m_moduleID.toString()
			+ " cannot be set"
		);
	}
}
