#include <cmath>

#include "model/SensorValue.h"

using namespace BeeeOn;

SensorValue::SensorValue():
	m_value(NAN),
	m_valid(false)
{
}

SensorValue::SensorValue(const ModuleID &moduleID):
	m_moduleID(moduleID),
	m_value(NAN),
	m_valid(false)
{
}

SensorValue::SensorValue(const ModuleID &moduleID, const double &value):
	m_moduleID(moduleID),
	m_value(value),
	m_valid(true)
{
}
